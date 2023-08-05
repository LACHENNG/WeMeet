// Author : lang @ nwpu
// All rights reserved

// FIXME: see how to customize QListWidget, see https://doc.qt.io/qt-5/stylesheet-reference.html#list-of-stylable-widgets
#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QFrame>
#include <QMainWindow>
#include <src/codec/protobuf_fwd.h>
#include <ui/chatmessage/textmessage.h>
#include <ui/chatmessage/filemessage.h>
#include <src/protoc/message.pb.h>
#define Chinese(str) QString::fromLocal8Bit(str)

QT_BEGIN_NAMESPACE
namespace Ui { class ChatWindow; }
QT_END_NAMESPACE

class TcpClient;
class QListWidgetItem;
class CameraVideo;
namespace cv { class VideoCapture; };

class ChatWindow : public QMainWindow
{
    Q_OBJECT
public:
    ChatWindow(QWidget *parent = nullptr);
    ~ChatWindow(); // force out-line

private slots:
    // display a text message to QListWidget, time info displayed automatically if necessary
    void displayTextMessage(const QString& title,
                            const QString& text,
                            qint64 timeSecsSinceUnixEpoch,
                            TextMessage::TYPE userType);
    // display a file message to QListWidget, time info displayed automatically if necessary
    void displayFileMessage(const QString& title,
                            const QPixmap& fileIcon,
                            const QString& file_name_str,
                            qint64 file_size_bytes,
                            qint64 timeSecsSinceUnixEpoch,
                            FileMessage::TYPE type);

    // display a time message to QListWidget
    void displayTimeMessage(qint64 timeSecsSinceEpoch);
    // add time in listWight conditionally (if the interval gt 1 min)
    void mayDisplayTimeMessage(qint64 curMsgTimeSecsSinceUnixEpoch);

    void sendTextMessage(const QString& msg);
    // send file only less than 2GB, due to the implementation
    void sendFileMessage(const QString& file_path);

    // On protobuf::Message data  arrived
    void onProtoMessageReceived(MessagePtr msg);
    // Dispatching MeetChat::message
    void onTextMessage(QSharedPointer<MeetChat::Message> message);
    void onFileMessage(QSharedPointer<MeetChat::Message> message);

    void on_sendButton_clicked();
    void on_fileButton_clicked();
    void on_videoButton_clicked();
    void on_soundButton_clicked();

private slots:



    // all event connect related stuff is done here
    void connectEventSlots();

private:
    Ui::ChatWindow*ui;
    CameraVideo* m_cameraVideo;
    QScopedPointer<TcpClient> m_chatClient;
};

#endif // CHATWINDOW_H
