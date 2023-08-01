// Author : Lance @ nwpu
//
#pragma once

#include <QObject>
#include <memory>
#include "protobuf_fwd.h"

class TcpClient;

class ProtobufCodec : public QObject
{
    Q_OBJECT

public:
    // create (reflect) a message by TypeName and decode the
    // binary data(which previously serialized by protobuf::Message::serialzexxx) to fill the Message
    // at protoData with lenght `size`
    static MessagePtr makeProtoMessageFromProtodataArray(const StringPiece& msgType, const char* protoData, size_t size);

    ProtobufCodec(TcpClient* tcpclient);

    // note that return a empty QByteArray when fails
    QByteArray encodeMessage(const Message& message);

public slots:
    void onRawBytes();

signals:

    void rawBytesDecoded(MessagePtr);

private:
    // read and discared from QTcpSocket
    void hasRead(qint64 n);
    // note this make a copy of current input buffer of QTcpSocket without a side-effect
    QByteArray peekBytes(qint64 n);
    // hold a tcp_client pointer to read and send data
    TcpClient* m_tcpPtr;  // QTcpSocket
};




