// Author : lang(Lance) @ nwpu transplus@foxmail.com
// All rights reserved

#pragma once

#include <QObject>
#include <memory>
#include "protobuf_fwd.h"

class TcpClient;

// ProtobufCodec encodes(packs) a probuf::Message to Packet( see class Packet to see how the packet is designed)
//    or decodes raw network bytes to Packet, then, further to probuf::Message ( emit rawBytesDecoded signal if
//    a decoded probuf::Message is ready
// ---raw bytes----> onRawBytes() -- > rawBytesDecoded(ProtoMessagePtr)
// ---raw bytes-<--- encodeMessage(ProtoMessage)
class ProtobufCodec : public QObject
{
    Q_OBJECT

public:
    // create (reflect) a message by TypeName and decode the
    // binary data(which previously serialized by protobuf::Message::serialzexxx) to fill the Message
    // at protoData with lenght `size`
    static ProtoMessagePtr makeProtoMessageFromProtodataArray(const StringPiece& msgType, const char* protoData, size_t size);

    ProtobufCodec(TcpClient* tcpclient);

    // note that return a empty QByteArray when fails
    QByteArray encodeMessage(const ProtoMessage& message);

public slots:
    void onRawBytes();

signals:
    void rawBytesDecoded(ProtoMessagePtr);

private:
    // read and discared from QTcpSocket
    void hasRead(qint64 n);
    // note this make a copy of current input buffer of QTcpSocket without a side-effect
    QByteArray peekBytes(qint64 n);
    // hold a tcp_client pointer to read and send data
    TcpClient* m_tcpPtr;  // QTcpSocket
};




