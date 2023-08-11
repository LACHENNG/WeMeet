﻿#include "cameravideo.h"
#include <QLabel>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <QDebug>
#include <opus.h>
#include "protoc/message.pb.h"
#include "src/mediacodec.h"
#include "src/config.h"

CameraVideo::CameraVideo(QWidget* showWhere, int fps,  QWidget *parent)
    : m_showWhere(showWhere),
    m_fps(fps),
    m_pixmapLable(new QLabel(showWhere)),
    m_timerCameraFrame(new QTimer(parent)),
    m_cap(new cv::VideoCapture()),
    m_mediaCodec(new MediaCodec(MediaCodec::USE_AS_ENCODER)),
    m_mediaDecoder(new AVDecoderMuxer())
{
    this->setFps(fps);
    connectSlots();
}

CameraVideo::~CameraVideo()
{
    delete m_timerCameraFrame;
}

bool CameraVideo::startCap()
{
    m_cap->open(std::stoi(Config::getInstance().get("cameraIdx")));
    cv::Mat frame;
    bool open_success = m_cap->read(frame);
    if(open_success) m_timerCameraFrame->start();
    return open_success;
}

void CameraVideo::processOneAVFrame()
{
    cv::Mat frame;
    *m_cap >> frame;
    if(frame.empty()){
        stopCap();
        return ;
    }
    displayCVMat(frame, m_showWhere);
    // has to resize to satisfy the configured mediaCodec
    resize(frame, frame, cv::Size(VIDEO_WIDTH, VIDEO_HEIGHT), 0, 0, cv::INTER_CUBIC);
    int rc = m_mediaCodec->encodeFrame(frame);
    if(rc != 0){
        qDebug() << "encode frame failed, rc = " << rc;
    }
}

void CameraVideo::connectSlots()
{
    connect(this->m_timerCameraFrame, SIGNAL(timeout()),
            this, SLOT(processOneAVFrame()));
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


void CameraVideo::setOnFrameEncodedCallback(const OnFrameEncodedCallback &cb)
{
    m_mediaCodec->setOnFrameEncodedCallback(cb);
}

void CameraVideo::setOnPacketDecodedCallback(const OnAVPacketDecodedCallback &cb)
{
   m_mediaDecoder->setOnPacketDecodedCallback(cb);
}

int CameraVideo::encodeFrame(const cv::Mat &mat)
{
    return m_mediaCodec->encodeFrame(mat);
}

int CameraVideo::decodeAVPacket(const QSharedPointer<MeetChat::Message>& av_message)
{
    return m_mediaDecoder->decodeAVPacket(av_message);
}

void CameraVideo::displayCVMat(const cv::Mat& mat, QWidget * showWhere)
{
    QImage image= QImage((const unsigned char*)(mat.data), mat.cols, mat.rows,
                          QImage::Format_RGB888).rgbSwapped();

    m_pixmapLable->setPixmap(QPixmap::fromImage(image)
                                 .scaledToWidth(showWhere->size().width(), Qt::SmoothTransformation));

    m_pixmapLable->resize(showWhere->size());
    m_pixmapLable->move(0, 0);
    m_pixmapLable->show();
}

