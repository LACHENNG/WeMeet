#include "tcpclient.h"
#include <google/protobuf/message.h>
#include <QDebug>
#include <QNetworkProxy>
#include <QtEndian>
#include <string>
#include <src/protoc/message.pb.h>


TcpClient::TcpClient(const QString &hostname,
                       quint16 port,
                       QObject *parent)
  : QTcpSocket(parent),
    m_codec(new ProtobufCodec(this))
{
    setPeerName(hostname);
    setPeerPort(port);
    connectEventSlots();
    setProxy(QNetworkProxy::NoProxy);
}

TcpClient::~TcpClient()
{
     disconnectFromHost();
}

void TcpClient::start()
{

     this->connectToHost(peerName(), peerPort());
}

void TcpClient::OnMessage(ProtoMessagePtr message)
{
     if(!message){
         qDebug() << "invalid protobuf message, discarded...";
         return ;
     }
     qDebug() << "OnMessage : " << message->GetTypeName().data();
     emit protobufMessage(message);
}


void TcpClient::OnConnected()
{
    qDebug() << "connected";
//     setSocketOption(QAbstractSocket::LowDelayOption, 1);
}

void TcpClient::OnDisConnected()
{
    qDebug() << "disconnected";
}

void TcpClient::send(const google::protobuf::Message& msg)
{
    QByteArray data = m_codec->encodeMessage(msg);
    if(data.size() == 0){
        qDebug() << "Warn: encodeMessage failed, skip send msg [ type name = "
                 << msg.GetTypeName().data() << " ] ";
    }
    write(data);
}

void TcpClient::onError(QAbstractSocket::SocketError errCode)
{
    //Note: QAbstractSocket::RemoteHostClosedError
    //      will emit dissconnected too.
    qDebug()<< "OnError : " << QAbstractSocket::errorString()
            << "code : " << errCode;
}

void TcpClient::connectEventSlots()
{
    // connect sigle onConnection, onDisconnection, onMessage
    connect(this, SIGNAL(connected()), this, SLOT(OnConnected()));
    connect(this, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onError(QAbstractSocket::SocketError)));
    connect(this, SIGNAL(disconnected()),
            this, SLOT(OnDisConnected()));
    connect(this, SIGNAL(readyRead()),
            m_codec.get(), SLOT(onRawBytes()));
    connect(m_codec.get(), SIGNAL(rawBytesDecoded(ProtoMessagePtr)),
            this, SLOT(OnMessage(ProtoMessagePtr)) );
}


