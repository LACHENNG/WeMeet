// Author : lang @ nwpu transplus@foxmail.com
// All rights reserved

#ifndef CAMERA_VIDIO_H
#define CAMERA_VIDIO_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <src/protoc/message.pb.h>
#include "avmessagemuxer.h"

#include <QObject>

// forward declaration
namespace MeetChat{ class VideoFrame; }
namespace cv { class VideoCapture; class Mat; }
class QLabel;
class MediaCodec;


// Used to open and show camera vedio to the user specified QWidget
// and encode and decode audio / video data (the encoded or decoded data is passed to user by callback mechanism)
// It use QTimer to control FPS
class CameraVideo : public QObject{
    Q_OBJECT
public :
    CameraVideo(QWidget* showWhere = nullptr, int fps = 30, QWidget *parent = nullptr);
    ~CameraVideo();

public:
    using OnFrameEncodedCallback = std::function<void (const MeetChat::AVPacket&)>;
    using OnAVPacketDecodedCallback = std::function<void (const MessageContext&, const AVFrame*)>;
    // start capture and send camera frame
    // return true if success
    bool startCap();
    void stopCap();
    void setFps(int Fps);

    void setOnFrameEncodedCallback(const OnFrameEncodedCallback& cb);
    void setOnPacketDecodedCallback(const OnAVPacketDecodedCallback& cb);

    // encode one Frame
    // Note: setOnFrameEncodedCallback to received encoded data
    int encodeFrame(const cv::Mat& mat);
    // decode one MeetChat::AVPacket ( encoded by encodeFrame)
    // Note: setOnPacketDecodedCallback to received decoded data
    int decodeAVPacket(const QSharedPointer<MeetChat::Message>& av_message);

    void displayCVMat(const cv::Mat& in_mat, QWidget * show_where);

private slots:
    // process single frame capture from camera
    // timeout function for QTimer
    void processOneAVFrame();

    void connectSlots();

private:
    QWidget* m_showWhere = nullptr;
    int m_fps;
    int m_cameraIdx;
    QLabel* m_pixmapLable = nullptr;
    QTimer* m_timerCameraFrame = nullptr;
    std::unique_ptr<cv::VideoCapture> m_cap;
    std::unique_ptr<MediaCodec> m_mediaCodec;
    std::unique_ptr<AVDecoderMuxer> m_mediaDecoder;
};

#endif
