// Author : lang @ nwpu
// All rights reserved

#ifndef CAMERA_VIDIO_H
#define CAMERA_VIDIO_H

#include <QObject>

class QLabel;
namespace cv { class VideoCapture; };

// used to open and show camera vedio to the user specified QWidget
class CameraVideo : public QObject{
    Q_OBJECT
public :
    CameraVideo(QWidget* showWhere = nullptr, int fps = 30,
                int cameraIdx = 0, QWidget *parent = nullptr);
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
    cv::VideoCapture* m_cap;
};

#endif
