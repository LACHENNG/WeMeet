﻿#include "packet.h"
#include <google/protobuf/stubs/stringpiece.h>
#include <google/protobuf/message.h>

#include <cstdint>
#include <cstdlib>
#include <QDebug>
#include <winsock.h>

#include <src/config.h>


bool Packet::canParseFromArray(const void* packet_header, size_t readableBytes)
{
    if( readableBytes < kHeaderLen) return false;

    int32_t belen = *static_cast<const int32_t*>(packet_header);
    int32_t payload_len = ntohl(belen);
    // FIXME
    if(payload_len < 0 || payload_len > UINT32_MAX - 4){
        qDebug() << "Invalid packet payload len = " << payload_len;
        return false;
    }
    if(readableBytes < kHeaderLen + payload_len){
        return false;
    }

    return true;
}


Packet Packet::parseFromArray(const void *start, size_t len)
{ 
    // struct Proto{
    //     const int32_t m_headerBe32;
    //     const int32_t m_msglenBe32;
    //     const char m_typeName[m_msglenBe32];
    //     const char m_payload[m_headerBe32 - m_msglenBe32 - 4 - 4];
    //     const int32_t m_checkSumBe32;
    // };
    Packet packet;
    assert(packet.m_state == Uninstallized);
    packet.m_headerBe32 = static_cast<const int32_t*>(start); 
    packet.m_msglenBe32 = (const int32_t*)((const char*)packet.m_headerBe32 + sizeof(kHeaderLen));
    packet.m_typeName = (const char*)packet.m_msglenBe32 + sizeof(kMsgNameLen);
    int msglen = ntohl(*(packet.m_msglenBe32));
    packet.m_payload = packet.m_typeName + msglen; // 1 for '\0'
    packet.m_checkSumBe32 = reinterpret_cast<const int32_t*>((const char*) start + len - sizeof(kCheckSumLen));

    packet.m_state = Parsed;

    return packet;
}


bool Packet::packProtoMessageToCachedSizeArray(void * start, size_t buf_size, const ProtoMessage& message){
    std::string typeName = message.GetTypeName();
    int packetBytesAll = kHeaderLen + kMsgNameLen + kCheckSumLen + typeName.size() + 1 + message.ByteSizeLong(); // 1 for '\0'
    if(static_cast<int>(buf_size) < packetBytesAll){
        qDebug() << "buf_size < packetBytes All" << " buf_size : " <<buf_size << " packetBytesAll : " << packetBytesAll;
        return false; // cached buffer size is not big enough to hold serialized data
    }

    int32_t headerlenBe32 = htonl(packetBytesAll - kHeaderLen);
    // LOG_DEBUGC << "headerlenBe32 "<< headerlenBe32;
    int32_t msglenBe32 = htonl(typeName.size() + 1);
    // LOG_DEBUGC << "msglenBe32 "<< msglenBe32;
    int32_t checkSumBe32 = htonl(calcCheckSum(typeName, message));

    char* dest = (char*)start;
    *((int32_t*)(dest)) = headerlenBe32;
    // memcpy(dest, &headerlenBe32, sizeof(int32_t));
    dest += sizeof(headerlenBe32);
    *((int32_t*)(dest)) = msglenBe32;
    // memcpy(dest, &msglenBe32, sizeof(int32_t));

    dest += sizeof(msglenBe32);
    dest = std::copy(typeName.begin(), typeName.end(), dest);
    *dest++ = '\0';
    char* begin = dest;
    dest = (char*)message.SerializeWithCachedSizesToArray((uint8_t*)dest);
    assert(dest - begin == static_cast<int>(message.ByteSizeLong()));

    *(reinterpret_cast<int32_t*>(dest)) = checkSumBe32;
    dest += sizeof(checkSumBe32);

    assert(dest - (char*)start == packetBytesAll);
    if(Config::getInstance().get("displayPacketSend") == "1"){
        qDebug() << "++++++++++++++++Packet ++++++++++++++++++++++++++\n"
                 << "(4B) Header len : " << packetBytesAll - kHeaderLen << "\n"
                 << "(4B) Msg name len: " << typeName.size() + 1 << "\n"
                 << "Msg name: " << typeName.c_str() << "\n"
                 << "Protodata with size: " << message.ByteSizeLong() << "\n"
                 << "(4B)Check sum : " << calcCheckSum(typeName, message) << "\n"
                 << "+++++++++++++++++++++++++++++++++++++++++++++++++";
    }

    return (dest - (char*)start == packetBytesAll);
}

size_t Packet::bytesAllToPack(const google::protobuf::Message &message)
{
    return kHeaderLen + kMsgNameLen + kCheckSumLen + message.GetTypeName().size() + 1 + message.ByteSizeLong();
}

void Packet::printPacket(const Packet &packet)
{
    qDebug() << "+++++++++++++Packet show in host order++++++++++++\n"
             << "(4B) Header len : " << packet.headerLen() << "\n"
             << "(4B) Msg name len: " << packet.typeNameLen() + 1 << "\n"
             << "Msg name: " << packet.messageTypeName() << "\n"
             << "Protodata with size: " << packet.headerLen() - packet.typeNameLen() - 1 - kCheckSumLen << "\n"
             << "(4B)Check sum : " << packet.checkSum() << "\n"
             << "+++++++++++++++++++++++++++++++++++++++++++++++++";
}
