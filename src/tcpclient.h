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
// use to send and receive data from peer
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
    void OnMessage(MessagePtr message);
    void OnConnected();
    void OnDisConnected();
    // sending
    void send(const Message& msg);

signals:
    // signal GUI
    void protobufMessage(MessagePtr message);

private slots:
    void onError(QAbstractSocket::SocketError);

private:
    void connectEventSlots();

    std::unique_ptr<ProtobufCodec> m_codec;

};

#endif // TCPCLIENT_H
