// Author : lang @ nwpu
// All rights reserved

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QMainWindow>
#include <src/codec/protobuf_fwd.h>
#include <ui/chatmessage/qnchatmessage.h>

QT_BEGIN_NAMESPACE
namespace Ui { class ChatWindow; }
QT_END_NAMESPACE

class TcpClient;
class QListWidgetItem;

class ChatWindow : public QMainWindow
{
    Q_OBJECT
public:
    ChatWindow(QWidget *parent = nullptr);
    ~ChatWindow(); // force out-line

protected:
//    void resizeEvent(QResizeEvent *event);

private slots:
    // and message to QListWidget, time info displayed automatically if necessary
    void addMessage(QString text, QString timeSecsSinceUnixEpoch, QNChatMessage::User_Type userType);
    void sendTextMessage(const QString& msg);

    // on protobuf::Message data  arrived
    void onProtoMessageReceived(MessagePtr msg);
    void on_sendButton_clicked();

private:
    void addMessageItemTolistWidget(QString text, QString timeSecsSinceUnixEpoch, QNChatMessage::User_Type userType);

    // add time in listWight conditionally (if the interval gt 1 min)
    void mayAddTimeTolistWidget(QString curMsgTimeSecsSinceUnixEpoch);

    void connectEventSlots();
private:
    Ui::ChatWindow*ui;
    QScopedPointer<TcpClient> m_chatClient;
};
#endif // CHATWINDOW_H
