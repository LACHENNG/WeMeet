#include "cameravideo.h"
#include <QLabel>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <QDebug>
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
