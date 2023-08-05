#include "protobufCodec.h"
#include "src/tcpclient.h"
#include "packet.h"
#include <cstdint>
#include <cassert>
#include <google/protobuf/message.h>

ProtobufCodec::ProtobufCodec(TcpClient *tcpclient)
    : m_tcpPtr(tcpclient)
{
}

QByteArray ProtobufCodec::encodeMessage(const Message &message)
{
    QByteArray buffer;
    buffer.resize(Packet::bytesAllToPack(message));
    bool ok = Packet::packProtoMessageToCachedSizeArray(buffer.data(), buffer.size(), message);
    if(!ok) buffer.resize(0);
    // RVO
    return buffer;
}


void ProtobufCodec::onRawBytes()
{
    //FIXME: this may helpful to use to implement progress bar? eg. on file download or so?
    static int i = 1;
    qDebug() << i++ << " on Raw Bytes";

    while(Packet::canParseFromArray(peekBytes(Packet::kHeaderLen).data(), this->m_tcpPtr->bytesAvailable()))
    {
         QByteArray buffer =  m_tcpPtr->read( this->m_tcpPtr->bytesAvailable() );
        // parse a packet
        Packet packet = Packet::parseFromArray(buffer.data(),  buffer.size());
        // reflect a Message from packet
        const char* msgType = packet.messageTypeName();
        const char* payload = packet.payload();
        MessagePtr message =
            makeProtoMessageFromProtodataArray(msgType, payload, packet.payloadLen());
        emit rawBytesDecoded(message);
    }
}

void ProtobufCodec::hasRead(qint64 n)
{
    m_tcpPtr->read(n);
}

QByteArray ProtobufCodec::peekBytes(qint64 n)
{
    QByteArray bytes;
    bytes.resize(n);
    m_tcpPtr->peek(bytes.data(), n);
    return bytes;
}

MessagePtr ProtobufCodec::makeProtoMessageFromProtodataArray(const StringPiece &msgType, const char *protoData, size_t size)
{
    // using protobuf reflect to create Message
    MessagePtr message;
    const auto descriptor =  google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(msgType.begin());
    if(descriptor){
        const Message* prototype =
            google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if(prototype) message.reset(prototype->New());
    }

    if(!message){
        return message;
    }
    // fill Mesage
    bool f = message->ParseFromArray(protoData, size);
    if(!f) message.reset(); // parse failed
    return message;
}

