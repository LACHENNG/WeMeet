// Author : lang @ nwpu transplus@foxmail.com
// All rights reserved

#ifndef AUDIO_VIDIO_CTRL_H
#define AUDIO_VIDIO_CTRL_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <src/protoc/message.pb.h>


#include <QObject>

// forward declaration
namespace MeetChat{ class VideoFrame; }
namespace cv { class VideoCapture; class Mat; }
class QLabel;
class MediaCodec;
class PortAudioX;
class AudioPlayer;
class AVDecoderMuxer;
class MessageContext;

// AVControl: audio video control
// Used to open and show camera vedio to the user specified QWidget
// and encode and decode audio / video data (the encoded or decoded data is passed to user by callback mechanism)
// It use QTimer to control FPS
class AVControl : public QObject{
    Q_OBJECT
public :
    AVControl(QWidget* showWhere = nullptr, int fps = 30, QWidget *parent = nullptr);
    ~AVControl();

public:
    using OnFrameEncodedCallback = std::function<void (const MeetChat::AVPacket&)>;
    using OnAVPacketDecodedCallback = std::function<void (const MessageContext&, const AVFrame*)>;
    // start capture and send camera frame
    // return true if success
    bool startCap();

    // start caputre audio and
    // return false if failed
    bool startCapAudio();

    void stopCap();
    void stopCapAudio();

    void setFps(int Fps);

    // set callback to receive encoded AVPacket which contains encoded
    // audio or video data
    void setOnFrameEncodedCallback(const OnFrameEncodedCallback& cb);

    // set callback to receive decoded AVFrame which contains decoded
    // audio or vidio data
    void setOnPacketDecodedCallback(const OnAVPacketDecodedCallback& cb);

    // display cv::Mat to given QWidget
    void displayCVMat(const cv::Mat& in_mat, QWidget * show_where);

    // MT-Safe
    // Queue audio to audio playing queue
    // FIXME: if the client invoke queueToPlay too fast, the internal buffer may consume to much memory
    void queueToPlay(void *audio_src, int src_bytes);

    // decode one MeetChat::AVPacket ( encoded by encodeFrame)
    // Note: setOnPacketDecodedCallback to received decoded data
    int decodeAVPacket(const QSharedPointer<MeetChat::Message>& av_message);

private:
    // encode one Frame
    // Note: setOnFrameEncodedCallback to received encoded data
    int encodeFrame(const cv::Mat& mat);

    // send audio(PCM) data to be encoded
    // Note: set proper callback to recevie encoded data
    int encodeFrame(const void* audio_data, unsigned long nSampleNum,
                    size_t nBytesPerSample, uint8_t n_channel);
private slots:
    // try to open camera, return false on failure
    bool openCamera();

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
    std::unique_ptr<AudioPlayer> m_audio_output;
    std::unique_ptr<PortAudioX> m_audio_input;
    std::unique_ptr<MediaCodec> m_mediaCodec;
    std::unique_ptr<AVDecoderMuxer> m_mediaDecoder;
};

#endif // AUDIO_VIDIO_CTRL_H
