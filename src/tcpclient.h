// Author : lang @ nwpu
// All rights reserved

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include "codec/protobufCodec.h"
#include <src/codec/protobuf_fwd.h>

class ProtobufCodec;

// provide core events callbacks in network programing
//    eg. OnConnect, OnDissConnect, OnMessage
// Also provide fuctionality to send probuf::Message data to peer
class TcpClient : public QTcpSocket
{
    Q_OBJECT
public:
    explicit TcpClient(const QString & hostname,
                        quint16 port,
                        QObject *parent = nullptr);
    ~TcpClient();
    void start();

public slots:
    void OnMessage(ProtoMessagePtr message);
    void OnConnected();
    void OnDisConnected();

    void send(const ProtoMessage& msg);

signals:
    // signal GUI
    void protobufMessage(ProtoMessagePtr message);

private slots:
    void onError(QAbstractSocket::SocketError);

private:
    void connectEventSlots();
    // use to pack probuf::Message to Packet or unpack Packet to probuf::Message
    std::unique_ptr<ProtobufCodec> m_codec;
};

#endif // TCPCLIENT_H
