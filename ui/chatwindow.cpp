#include "chatwindow.h"
#include "src/av_decoder_muxer.h"
#include "ui_chatwindow.h"
#include "src/tcpclient.h"
#include <src/av_control.h>
#include "src/config.h"
#include <QDateTime>
#include <src/protoc/message.pb.h>

#include <QFileDialog>
#include <QFileIconProvider>
#include <QMessageBox>

#include <ui/chatmessage/messagecommon.h>
#include <ui/chatmessage/timemessage.h>
#include <ui/chatmessage/textmessage.h>
#include <ui/chatmessage/filemessage.h>

#include <opencv2/opencv.hpp>
#include <QDesktopWidget>
#include <src/mediacodec.h>

ChatWindow::ChatWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ChatWindow),
    m_chatClient(new TcpClient(QString::fromStdString(Config::getInstance().get("server_address")),
                               stoi(Config::getInstance().get("server_port")),
                               parent))
{
    ui->setupUi(this);
    ui->splitter->handle(1)->setAttribute(Qt::WA_Hover, true);
    ui->listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // construct CameraVideo after ui->setupUi
    m_cameraVideo = new AVControl(ui->displayWidget, 30, parent),
    m_chatClient->start();

    this->setWindowTitle(Chinese("MeetChat 欢迎 @ ") + QString::fromStdString(Config::getInstance().get("userId")));
    connectEventSlotsOrCallbacks();

    centralizeThisWind(390, 620);

    qRegisterMetaType<MeetChat::AVPacket>("MeetChat::AVPacket");
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::sendTextMessage(const QString &msg)
{
    MeetChat::Message message;
    message.set_sender_id(Config::getInstance().get("userId"));
    message.set_receiver_id("-1");
    message.mutable_timestamp()->set_seconds(QDateTime::currentSecsSinceEpoch());
    message.set_type(MeetChat::MessageType::TEXT);

    MeetChat::TextMessage text_message;
    std::string stdstr = msg.toStdString();
    text_message.set_text(stdstr);

    google::protobuf::Any *any = new google::protobuf::Any;
    any->PackFrom(text_message);
    message.set_allocated_data(any);

    m_chatClient->send(message);
}

void ChatWindow::sendFileMessage(const QString &file_path)
{
    QFile file(file_path);
    QFileInfo fileInfo(file.fileName());
    QString fileName = fileInfo.fileName();
    file.open(QIODevice::ReadOnly);
    if(file.size() >= (long long)2 * 1024 * 1024 * 1024) // 2GB
    {
        qDebug() << "Too big, ignore file";
        QMessageBox msgBox;
        msgBox.setWindowTitle(Chinese("警告"));
        msgBox.setText(Chinese("无法发送文件，文件太大 ( > 2GB )"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec(); // wait user to click
        return ;
    }
    // file.readAll() only reads data less that 2GB
    std::string data(file.readAll().toStdString());

    MeetChat::File file_message;
    file_message.set_name(fileName.toStdString());
    file_message.set_size(file.size());
    assert(file.size() == data.size());
    file_message.set_data(data.data(), file.size());

    google::protobuf::Any *any = new google::protobuf::Any;
    bool f = any->PackFrom(file_message);
    if(!f){
        qDebug() << "packfrom error";
        return ;
    }

    MeetChat::Message message;
    message.set_sender_id(Config::getInstance().get("userId"));
    message.set_receiver_id("all");
    message.mutable_timestamp()->set_seconds(QDateTime::currentSecsSinceEpoch());
    message.set_type(MeetChat::MessageType::FILE);
    message.set_allocated_data(any);

    m_chatClient->send(message);
}

void ChatWindow::sendAVMessgae(const MeetChat::AVPacket &av_packet)
{
    MeetChat::Message message;
    message.set_sender_id(Config::getInstance().get("userId"));
    message.set_receiver_id("all");
    message.mutable_timestamp()->set_seconds(QDateTime::currentSecsSinceEpoch());
    message.set_type(MeetChat::MessageType::AV);

    google::protobuf::Any *any = new google::protobuf::Any;
    any->PackFrom(av_packet);
    message.set_allocated_data(any);

    m_chatClient->send(message);
}

void ChatWindow::onProtoMessageReceived(ProtoMessagePtr baseMessage)
{
    QSharedPointer<MeetChat::Message> message = baseMessage.dynamicCast<MeetChat::Message>();
    switch (message->type()) {
        case MeetChat::MessageType::TEXT:
            onTextMessage(message);
            break;
        case MeetChat::MessageType::FILE:
            onFileMessage(message);
            break;
        case MeetChat::MessageType::AV:
            onAVMessage(message);
            break;
        default:
            std::cerr << "Unknown message type\n";
        }
}

void ChatWindow::onTextMessage(const QSharedPointer<MeetChat::Message> &message)
{
    assert(message->type() == MeetChat::TEXT);
    google::protobuf::Any any = message->data();
    MeetChat::TextMessage text_message;
    if (any.UnpackTo(&text_message)) {
        std::string text = text_message.text();
        qDebug() << QString::fromStdString(text);
        displayTextMessage(QString::fromStdString(message->sender_id()),
                           QString::fromStdString(text),
                           QDateTime::currentSecsSinceEpoch(),
                           TextMessage::OTHER);
    } else {
        // can not parse
        std::cerr << "Can`t parse data to TEXT type\n";
    }
}

void ChatWindow::onFileMessage(const QSharedPointer<MeetChat::Message>& message)
{
    assert(message->type() == MeetChat::FILE);
    google::protobuf::Any any = message->data();
    MeetChat::File file_message;
    if (any.UnpackTo(&file_message)) {
        qDebug() << "received file name: " << file_message.name().c_str();

        QFile savefile(QString::fromStdString("./tmp/" + file_message.name()));
        QDir dir("./tmp");
        if (!dir.exists()) {
            dir.mkpath("./");
        }
        savefile.open(QIODevice::WriteOnly);
        savefile.write(file_message.data().data(), file_message.size());

        QFileInfo file_info(QString::fromStdString(file_message.name()));
        QFileIconProvider icon_provider;
        QIcon icon = icon_provider.icon(file_info);
        displayFileMessage(QString::fromStdString(message->sender_id()),
                           icon.pixmap(10,10),
                           file_info.fileName(),
                           file_message.size(),
                           QDateTime::currentSecsSinceEpoch(),
                           FileMessage::OTHER);
    } else {
        std::cerr << "Can`t parse data to FILE type\n";
    }
}

void ChatWindow::onAVMessage(const QSharedPointer<MeetChat::Message>& message)
{
    assert(message->type() == MeetChat::AV);
    this->m_cameraVideo->decodeAVPacket(message);
}

void ChatWindow::displayTextMessage(const QString& title,
                        const QString& text,
                        qint64 timeSecsSinceUnixEpoch,
                        TextMessage::TYPE userType)
{
    mayDisplayTimeMessage(timeSecsSinceUnixEpoch);

    TextMessage* message = new TextMessage(userType, ui->listWidget);
    QListWidgetItem* itemTime = new QListWidgetItem(ui->listWidget);

    message->setContent(text);
    message->setTitle(title);
    message->setMsgTime(timeSecsSinceUnixEpoch);

    QSize size = QSize(this->ui->listWidget->width(), message->height());
    message->resize(size);
    itemTime->setSizeHint(size);
    ui->listWidget->setItemWidget(itemTime, message);

}

void ChatWindow::on_sendButton_clicked()
{
    QString msg = ui->textInput->toPlainText();
    ui->textInput->setText("");
    if(msg.isEmpty()){
        ui->textInput->setPlaceholderText("can not send empty or blank message");
        return ;
    }

    displayTextMessage(QString::fromStdString(Config::getInstance().get("userId")),
                       msg,
                       QDateTime::currentSecsSinceEpoch(),
                       TextMessage::ME);
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
    displayFileMessage(QString::fromStdString(Config::getInstance().get("userId")),
                       icon.pixmap(10,10),
                       file_info.fileName(),
                       file_info.size(),
                       QDateTime::currentSecsSinceEpoch(),
                       FileMessage::ME);

    sendFileMessage(filepath);
}

void ChatWindow::on_videoButton_clicked()
{
    enum Status{opened, closed};
    static Status status = closed;

    if(status == opened){ // current state is opend, try to close
        m_cameraVideo->stopCap();
        status = closed;
        ui->videoButton->setText(QString::fromLocal8Bit("开启视频"));
        return;
    }
    // current state is closed, try to open
    bool f = m_cameraVideo->startCap();
    if(f){
        ui->videoButton->setText(QString::fromLocal8Bit("关闭视频"));
        status = opened;
    }else{
        QMessageBox::warning(this, Chinese("错误！"),Chinese("摄像头被占用，无法打开"), QMessageBox::Yes);
    }
}

void ChatWindow::on_soundButton_clicked()
{
    enum Status{opened, closed};
    static Status status = closed;

    if(status == opened){ // current state is opend, try to close
        m_cameraVideo->stopCapAudio();
        status = closed;
        ui->soundButton->setText(QString::fromLocal8Bit("开启声音"));
        return;
    }
    // current state is closed, try to open
    bool f = m_cameraVideo->startCapAudio();
    if(f){
        ui->soundButton->setText(QString::fromLocal8Bit("关闭声音"));
        status = opened;
    }else{
        QMessageBox::warning(this, Chinese("错误！"),Chinese("无法打开麦克风"), QMessageBox::Yes);
    }
}

void ChatWindow::slot_send_avframe(MeetChat::AVPacket packet) {
    this->sendAVMessgae(packet);
}

void ChatWindow::displayTimeMessage(qint64 timeSecsSinceEpoch)
{
    TimeMessage* messageTime = new TimeMessage(ui->listWidget);
    messageTime->setTime(timeSecsSinceEpoch);
    QListWidgetItem* itemTime = new QListWidgetItem(ui->listWidget);

    QSize size = QSize(this->ui->listWidget->width(), 30);
    messageTime->resize(size);
    itemTime->setSizeHint(size);
    ui->listWidget->setItemWidget(itemTime, messageTime);
}

void ChatWindow::displayFileMessage(const QString& title,
                                    const QPixmap& fileIcon,
                                    const QString& file_name_str,
                                    qint64 file_size_bytes,
                                    qint64 timeSecsSinceUnixEpoch,
                                    FileMessage::TYPE type)
{
    mayDisplayTimeMessage(timeSecsSinceUnixEpoch);

    FileMessage* message = new FileMessage(type, ui->listWidget);
    QListWidgetItem* itemfile = new QListWidgetItem(ui->listWidget);

    message->setTitle(title);
    message->setIcon(fileIcon);
    message->setFileNameStr(file_name_str);
    message->setFileSize(file_size_bytes);
    message->setMsgTime(timeSecsSinceUnixEpoch);

    QSize size = QSize(message->width(), 60);
    message->resize(size);

    itemfile->setSizeHint(size);
    ui->listWidget->setItemWidget(itemfile, message);
}

void ChatWindow::connectEventSlotsOrCallbacks()
{
    connect(m_chatClient.get(), SIGNAL(protobufMessage(ProtoMessagePtr)),
            this, SLOT(onProtoMessageReceived(ProtoMessagePtr)));

    connect(this, SIGNAL(signal_avframe_encoded(MeetChat::AVPacket)),
            this, SLOT(slot_send_avframe(MeetChat::AVPacket)));

    this->m_cameraVideo->setOnFrameEncodedCallback([this](const MeetChat::AVPacket& data){
        // this callback may invoked in other thread (eg. PortAudio)
        //  send data(which use Qt socket) in other thread is not allowed
        // this->sendAVMessgae(data);
       emit signal_avframe_encoded(data);
    });


    // set on AVPacket decoded callback, AVPacket may contains audio or video data
    // which requires additional check to dispatch audio and video
    this->m_cameraVideo->setOnPacketDecodedCallback([this](const MessageContext& ctx, const AVFrame* m_av_frame){
        // according to the facts that video frame has nono zero width and height value

        if( m_av_frame->width != 0 && m_av_frame->height != 0 ){
            cv::Mat output_mat(m_av_frame->height, m_av_frame->width, CV_8UC3);
            MediaCodec::avFrame2cvMat(output_mat, const_cast<AVFrame*>(m_av_frame));

            static QLabel img_lable;
            QImage image= QImage((const unsigned char*)(output_mat.data), output_mat.cols, output_mat.rows,
                                  QImage::Format_RGB888).rgbSwapped();

            img_lable.setPixmap(QPixmap::fromImage(image)
                                         .scaledToWidth(640, Qt::SmoothTransformation));

            img_lable.resize(640,480);
            img_lable.setWindowTitle(  Chinese("我【") + QString::fromStdString(Config::getInstance().get("userName"))+ Chinese("】")
                                     + Chinese("正在观看来自 【") + QString::fromStdString(ctx.getSender_id()) + Chinese("】的视频分享"));
            img_lable.show();

            return ;
        }

        // AVFrame is audio otherwise
        void* start = m_av_frame->data[0];
        int nBytes = MediaCodec::getTotalBytesIn(m_av_frame);
               qDebug() << "About to queueToPlay" << nBytes <<  "nBytes";
        m_cameraVideo->queueToPlay(start, nBytes);
    });
}

void ChatWindow::centralizeThisWind(int height, int width)
{
    int screen_width = QApplication::desktop()->width();
    int screen_height = QApplication::desktop()->height();

    int x = (screen_width - 620) / 2;
    int y = (screen_height - 390) / 2;

    this->setGeometry(x, y, 620, 390);
}

void ChatWindow::mayDisplayTimeMessage(qint64 curMsgTimeSecsSinceEpoch)
{
    bool showTime = false;
    // compare with previous sended message time
    if(ui->listWidget->count() > 0){
        QListWidgetItem* lastItem = ui->listWidget->item(ui->listWidget->count() - 1);
        MessageCommon* messageW =  dynamic_cast<MessageCommon*>(ui->listWidget->itemWidget(lastItem));
        int lastTimeSecs = messageW->msgTimeSecs();
        int curTimeSecs = curMsgTimeSecsSinceEpoch;
        qDebug() << "curTime lastTime:" << curTimeSecs - lastTimeSecs;
        showTime = ((curTimeSecs - lastTimeSecs) > 60);
    }else{
    // the first message case
        showTime = true;
    }
    if(showTime){
        displayTimeMessage(curMsgTimeSecsSinceEpoch);
    }
}


