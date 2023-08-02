// Author : lang @ nwpu
// All rights reserved

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QFrame>
#include <QMainWindow>
#include <src/codec/protobuf_fwd.h>
#include <ui/chatmessage/qnchatmessage.h>

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

protected:
//    void resizeEvent(QResizeEvent *event);

private slots:
    // and message to QListWidget, time info displayed automatically if necessary
    void addMessage(QString text, QString timeSecsSinceUnixEpoch, QNChatMessage::User_Type userType);
    void sendTextMessage(const QString& msg);

    // on protobuf::Message data  arrived
    void onProtoMessageReceived(MessagePtr msg);
    void on_sendButton_clicked();
    void on_fileButton_clicked();
    void on_videoButton_clicked();
    void on_soundButton_clicked();

private slots:
    void addMessageItemTolistWidget(QString text, QString timeSecsSinceUnixEpoch, QNChatMessage::User_Type userType);

    // add time in listWight conditionally (if the interval gt 1 min)
    void mayAddTimeTolistWidget(QString curMsgTimeSecsSinceUnixEpoch);

    // all event connect related stuff is done here
    void connectEventSlots();

private:

    Ui::ChatWindow*ui;
    CameraVideo* m_cameraVideo;
    QScopedPointer<TcpClient> m_chatClient;
};

// used to open and show camera vedio to the user specified QWidget
class CameraVideo : public QObject{
    Q_OBJECT
public :
    CameraVideo(QWidget* showWhere = nullptr, int fps = 30, int cameraIdx = 0, QWidget *parent = nullptr);
public:
    // start camera with index cameraIdx and show it to QWidget* showWhere
    void startCap();
    // stop camera
    void stopCap();
    // set capture fps
    void setFps(int Fps);
    // set camera index (0 for the default, first camera)
    void setCameraIdx(int idx);

private slots:
    void readCameraFrame();

private:
    QWidget* m_showWhere;
    int m_fps;
    int m_cameraIdx;
    QLabel* m_pixmapLable;
    QTimer* m_timerCameraFrame;
    cv::VideoCapture* m_cap;// 创建一个VideoCapture对象，指定摄像头的索引号
};
#endif // CHATWINDOW_H
