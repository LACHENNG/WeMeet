// Author : Lance @ nwpu
// Taken from LanceNet library
// All rights reserved
#pragma once

#include <memory>
#include <cassert>
#include <winsock.h>
#include "protobuf_fwd.h"

// Packet Protocol is:
//
// struct ProtoPseudocode{
//     const int32_t m_headerBe32;
//     const int32_t m_msglenBe32;
//     const char m_typeName[m_msglenBe32];
//     const char m_payload[m_headerBe32 - m_msglenBe32 - 4 - 4];
//     const int32_t m_checkSumBe32;
// };
// Packet provide functionality to serialize/deserialize some data and pack or depack it
// with the above protocol
//
// Note that Packet do not manage memory but parse from array or serialize to arrary in-place,
// and it only hold pointers to user provided memory, make sure the data you provided outlives
// packet

// as we use a int32_t to represent message, the maximum payload is arround 2^31 - 1 = 2GB

// FIXME: packProtoMessageToCachedSizeArray() may cause error when protomessage is too big
// FIXME: checksum currently is ignored (not implemented)
class Packet{
private:
    Packet() : m_state(Uninstallized) {}

public:
    const static int32_t kHeaderLen = sizeof(int32_t);
    const static int32_t kMsgNameLen = sizeof(int32_t);
    const static int32_t kCheckSumLen = sizeof(int32_t);

    enum State{
        Uninstallized,
        Parsed
    };

    // Check if the memery in begin in start with len 
    // can form a Packet(the buffer must store Packages which should be guranteed by the caller)
    // must gurantee that memory is valid during parsing, the packet only holds pointer to original buffer
    static bool canParseFromArray(const void* start, size_t len);

    // the returned Packet do not copy the data, but hold pointer to usr provide memory in [start, start + len)
    // use carelly to avoid memory related errors
    static Packet parseFromArray(const void *start, size_t len);

    // Pack a Protobuf::Message to user cached size array
    static bool packProtoMessageToCachedSizeArray(void * start, size_t buf_size, const ProtoMessage& message);

    // calc bytes total to pack a `Protobuf::Message` to `Packet`
    static size_t bytesAllToPack(const ProtoMessage& message);

    // get packet header len in host byte order, has to be Parsed be this call
    int32_t headerLen() const { assert(m_state == Parsed); return ntohl(*m_headerBe32); }
    // get packet type name len in host byte order(not include the ending '\0'), has to be Parsed be this call
    // not that the actual tyename stored in packet always includes the ending '\0'
    int32_t typeNameLen() const{ assert(m_state == Parsed); return ntohl(*m_msglenBe32) - 1; } // not include the ending '\0'
                                                                                               //
    int32_t payloadLen() const{ assert(m_state == Parsed);return headerLen() - kMsgNameLen - typeNameLen() - 1 - kCheckSumLen;}

    // total bytes of parsed Packet
    int32_t byteSizeAll() const {
        return sizeof(kHeaderLen) + sizeof(kMsgNameLen) + sizeof(kCheckSumLen) + typeNameLen() + 1 + payloadLen();
    }

    const char* messageTypeName() const {assert(m_state == Parsed);return m_typeName; }

    const char* payload() const { assert(m_state == Parsed);return m_payload; }

    // get packet checksum in host byte order
    int32_t checkSum() const { assert(m_state == Parsed);return ntohl(*m_checkSumBe32); }

    static void printPacket(const Packet& packet);
private:
    // FIXME: imple adler32
    static int32_t calcCheckSum(const StringPiece& messageTypeName, const ProtoMessage& protoMessage) { return 0; }

    // check if the packet has the right checksum
    // FIXME : imple
    bool validateCheckSum() { return true; }


    State m_state;
    const int32_t* m_headerBe32 = nullptr;
    const int32_t* m_msglenBe32= nullptr;
    const char* m_typeName = nullptr;
    const char* m_payload = nullptr;
    const int32_t* m_checkSumBe32 = nullptr;
};
