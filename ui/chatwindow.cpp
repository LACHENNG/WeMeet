#include "chatwindow.h"
#include "ui_chatwindow.h"
#include "src/tcpclient.h"
#include "src/config.h"
#include <QDateTime>
#include <src/protoc/message.pb.h>
#include <ui/chatmessage/qnchatmessage.h>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QTimer>
#include <opencv2/opencv.hpp>
ChatWindow::ChatWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ChatWindow),
    m_chatClient(new TcpClient(Config::GetServerHostName(), Config::getServerPort(), parent))
{
    ui->setupUi(this);

    ui->splitter->handle(1)->setAttribute(Qt::WA_Hover, true);

    m_cameraVideo = new CameraVideo(ui->displayWidget, 30, 0, parent),

    m_chatClient->start();

    connectEventSlots();
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::sendTextMessage(const QString &msg)
{
    // 创建一个 TextMessage 对象，并设置文本内容
    MeetChat::Message message;
    message.set_sender_id("chenl");
    message.set_receiver_id("user2");
    message.mutable_timestamp()->set_seconds(time(NULL));
    message.set_type(MeetChat::MessageType::TEXT);

    MeetChat::TextMessage text_message;
    std::string stdstr = msg.toStdString();
    text_message.set_text(stdstr);

    google::protobuf::Any *any = new google::protobuf::Any;
    any->PackFrom(text_message);
    message.set_allocated_data(any);

    m_chatClient->send(message);
}

void ChatWindow::onProtoMessageReceived(MessagePtr base)
{
    QSharedPointer<MeetChat::Message> message = base.dynamicCast<MeetChat::Message>();
    if(!message){
        qErrnoWarning("Can not up-cast to MeetChat::Message");
    }
    auto type = message->type();
    google::protobuf::Any any = message->data();
    MeetChat::TextMessage text_message;

    switch (type) {
        case MeetChat::MessageType::TEXT:
            // 消息类型为 TEXT
            if (any.UnpackTo(&text_message)) {
                std::string text = text_message.text();
                addMessage(QString::fromStdString(text), QString::number(QDateTime::currentDateTime().toSecsSinceEpoch()), QNChatMessage::User_She);
            } else {
                // 无法解析出 TextMessage 对象，输出错误信息
                std::cerr << "Invalid data for TEXT type\n";
            }
            break;
        default:
            // 未知的消息类型，输出错误信息
            std::cerr << "Unknown message type\n";
    }

}

void ChatWindow::addMessage(QString text, QString timeSecsSinceUnixEpoch, QNChatMessage::User_Type userType)
{
    mayAddTimeTolistWidget(timeSecsSinceUnixEpoch);
    addMessageItemTolistWidget(text, timeSecsSinceUnixEpoch, userType);
}

void ChatWindow::on_sendButton_clicked()
{
    QString msg = ui->textInput->toPlainText();
    ui->textInput->setText("");
    if(msg.isEmpty()){
        ui->textInput->setPlaceholderText("can not send empty or blank message");
        return ;
    }
    addMessage(msg, QString::number(QDateTime::currentDateTime().toSecsSinceEpoch()), QNChatMessage::User_Me);
    sendTextMessage(msg);

}

void ChatWindow::on_fileButton_clicked()
{
    auto filepath = QFileDialog::getOpenFileName(this, tr("Open"), "");
    if(filepath.isEmpty())
        return ;
    // send file
    QFileInfo file_info(filepath);
    QFileIconProvider icon_provider;
    QIcon icon = icon_provider.icon(file_info);

    //QTextCursor cursor = ui->textInput->textCursor();
    //cursor.insertImage(icon.pixmap(64,64).toImage());
//    // 创建一个QLabel对象
//    QLabel *label = new QLabel();

//    // 使用QIcon的pixmap()方法获取图片的QPixmap对象
//    QPixmap pixmap = icon.pixmap(64, 64);

//    // 使用QLabel的setPixmap()方法设置图片
//    label->setPixmap(pixmap);

//    // 显示QLabel对象
//    label->show();


}



void ChatWindow::on_videoButton_clicked()
{
    static qint64 i = -1; i++;
    // open camera
    if(i % 2 == 0){
        ui->videoButton->setText(QString::fromLocal8Bit("关闭视频"));
        m_cameraVideo->startCap();
    }else{ // close camera
        ui->videoButton->setText(QString::fromLocal8Bit("开启视频"));
        m_cameraVideo->stopCap();
    }
}

void ChatWindow::on_soundButton_clicked()
{
    static qint64 i = -1; i++;
    // open camera
    if(i % 2 == 0){
        ui->soundButton->setText(QString::fromLocal8Bit("关闭声音"));

    }else{ // close camera
        ui->soundButton->setText(QString::fromLocal8Bit("开启声音"));
    }
}

void ChatWindow::connectEventSlots()
{
    connect(m_chatClient.get(), SIGNAL(protobufMessage(MessagePtr)),
            this, SLOT(onProtoMessageReceived(MessagePtr)));
}

void ChatWindow::addMessageItemTolistWidget(QString text, QString SecsSinceUnixEpoch, QNChatMessage::User_Type userType)
{
    QNChatMessage::User_Type type = QNChatMessage::User_Me;
    QNChatMessage* messageW = new QNChatMessage(ui->listWidget->parentWidget());
    QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
    messageW->setFixedWidth(this->ui->listWidget->width());
    QSize size = messageW->fontRect(text);
    item->setSizeHint(size);
    messageW->setText(text, SecsSinceUnixEpoch, size, userType);
    ui->listWidget->setItemWidget(item, messageW);
    messageW->setTextSuccess();
}

void ChatWindow::mayAddTimeTolistWidget(QString curMsgTime)
{
    bool showTime = false;
    // compare with previous sended message time
    if(ui->listWidget->count() > 0){
        QListWidgetItem* lastItem = ui->listWidget->item(ui->listWidget->count() - 1);
        QNChatMessage* messageW = (QNChatMessage*)ui->listWidget->itemWidget(lastItem);
        int lastTimeSecs = messageW->time().toInt();
        int curTimeSecs = curMsgTime.toInt();
        qDebug() << "curTime lastTime:" << curTimeSecs - lastTimeSecs;
        showTime = ((curTimeSecs - lastTimeSecs) > 60);
    }else{
    // the first message case
        showTime = true;
    }
    if(showTime){
        QNChatMessage* messageTime = new QNChatMessage(ui->listWidget->parentWidget());
        QListWidgetItem* itemTime = new QListWidgetItem(ui->listWidget);

        QSize size = QSize(this->ui->listWidget->width(), 40);
        messageTime->resize(size);
        itemTime->setSizeHint(size);
        messageTime->setText(curMsgTime, curMsgTime, size, QNChatMessage::User_Time);
        ui->listWidget->setItemWidget(itemTime, messageTime);
    }
}

CameraVideo::CameraVideo(QWidget* showWhere, int fps, int cameraIdx,  QWidget *parent)
    : m_showWhere(showWhere),
    m_fps(fps),
    m_cameraIdx(cameraIdx),
    m_pixmapLable(new QLabel(showWhere)),
    m_timerCameraFrame(new QTimer(parent)),
    m_cap(new cv::VideoCapture(cameraIdx))
{
    this->setFps(fps);
    this->setCameraIdx(cameraIdx);

    connect(this->m_timerCameraFrame, SIGNAL(timeout()),
            this, SLOT(readCameraFrame()));
}

void CameraVideo::startCap()
{
    m_cap->open(m_cameraIdx);
    m_timerCameraFrame->start();
}

void CameraVideo::readCameraFrame()
{
    using namespace cv;
    // 创建一个Mat对象，用于存储摄像头的帧
    Mat frame;
    *m_cap >> frame;

    QImage image= QImage((const unsigned char*)(frame.data), frame.cols, frame.rows,
                          QImage::Format_RGB888).rgbSwapped();

    m_pixmapLable->setPixmap(QPixmap::fromImage(image).
                             scaledToWidth(m_showWhere->size().width(), Qt::SmoothTransformation));
    m_pixmapLable->resize(m_showWhere->size());

    qDebug() << m_showWhere->size();
    m_pixmapLable->move(0, 0);
    m_pixmapLable->show();
}

void CameraVideo::stopCap()
{
    m_timerCameraFrame->stop();
    m_cap->release();
    m_pixmapLable->clear();
    m_pixmapLable->show();
}

void CameraVideo::setFps(int Fps)
{
    m_fps = std::max(1, Fps);
    bool active = m_timerCameraFrame->isActive();
    if(active){
        m_timerCameraFrame->stop();
    }
    m_timerCameraFrame->setInterval(1000 / Fps);

    if(active) m_timerCameraFrame->start();
}

void CameraVideo::setCameraIdx(int idx)
{
    m_cameraIdx = idx;
}
