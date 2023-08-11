// Author : lang(Lance) @ nwpu transplus@foxmail.com
// All rights reserved

#ifndef MEDIACODEC_H
#define MEDIACODEC_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
};

#include <src/protoc/message.pb.h>

namespace cv { class VideoCapture; class Mat; };

#define VIDEO_WIDTH 1024
#define VIDEO_HEIGHT 720
#define VIDEO_FPS 30
#define VIDEO_BIT_RATE 4000000    // bit rate of video
#define VIDEO_GOP_SIZE 10        // video key frame(I frame) interval, gap(#frames) between two I frames
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_CHANNELS 2
#define AUDIO_BIT_RATE 64000

// This MediaCodec encodes/decodes video and sounds
// to and from MeetChat::VideoFrame
// (see protoc/message.proto for more details of how this protobuf message is defined)
//
// It use H.264 to encode video frames
//    use Opus[ /ˈoʊpəs/，欧普斯 ] to encode sounds
//
// Important: do not use a single MediaCodec as a encoder and decoder at the same time
//            if you need encode and decode, make two seperate MediaCodec instance to do that
//            with different construct paramters
class MediaCodec
{
public:
    enum USE_PURPOSE{
        USE_AS_ENCODER,
        USE_AS_DECODER
    };
    // note data ptrs in MeetChat::AVPacket is guaranteed to be valid in the hold callback
    // do not hold copys of ptrs to internal data of AvPacket outlive this func scope
    using OnAVFrameEncodedCallback = std::function<void (const MeetChat::AVPacket&)>;
    using OnAVPacketDecodedCallback = std::function<void (const AVFrame*)>;

    MediaCodec(USE_PURPOSE usePurpose) { m_usePurpose = usePurpose; init(); }

    // return 0 if sucess, otherwise a negative error code is returnd;
    int32_t init();

    // callback on data encoded

    void setOnFrameEncodedCallback(const OnAVFrameEncodedCallback& cb) { m_onFrameEncodedCb = cb; }
    void setOnPacketDecodedCallback(const OnAVPacketDecodedCallback& cb){ m_onPacketDecodedCb = cb; }

    // encode one Frame
    // Note: setOnFrameEncodedCallback to received encoded data
    int encodeFrame(cv::Mat mat);
    // decode one MeetChat::Message which must has type == Av, which stores a MeetChat::AVPacket ( encoded by encodeFrame)
    // Note: setOnPacketDecodedCallback to received decoded data
    int decodeAVPacket(const MeetChat::AVPacket& av_packet);

    // convert cv::Mat to AvFrame using given SwsContext
    // if sws_ctx is left null, cv::Mat(assume BGR24 format) is convert to AVFrame (YUV420 format)
    static void cvMat2AVFrame(const cv::Mat& inMat, AVFrame* out_video_frame, SwsContext* sws_ctx = nullptr);

    // convert AvFrame to cv::Mat using given SwsContext
    // if sws_ctx is left null, AVFrame (assume YUV420 format) is convert to cv::Mat(BGR24)
    static void avFrame2cvMat(const cv::Mat& out_Mat, AVFrame* in_frame, SwsContext* sws_ctx = nullptr);

private:
    USE_PURPOSE m_usePurpose;
    // ffmpeg的AVFormatContext对象，用于输出h264格式的视频数据
    AVFormatContext* m_video_fmt_ctx = nullptr;
    // ffmpeg的AVCodec对象，用于编码h264格式的视频数据
    const AVCodec* m_video_codec = nullptr;
    // 初始化ffmpeg的AVCodecContext对象，用于设置编码参数
    AVCodecContext* m_video_codec_ctx = nullptr;

    // 初始化ffmpeg的AVStream对象，用于存储编码后的视频数据
    AVStream* m_video_stream = nullptr;

    // 初始化ffmpeg的SwsContext对象，用于转换图像格式
    SwsContext* m_sws_ctx = nullptr;

    // 初始化ffmpeg的AVFrame对象，用于存储原始图像数据
    AVFrame* m_video_frame = nullptr; // for encoding

    // 初始化ffmpeg的AVPacket对象，用于存储编码后的视频数据
    AVPacket* m_video_packet = nullptr; // for encoding

    // 初始化protobuf的VideoData对象，用于序列化视频数据
    OnAVFrameEncodedCallback m_onFrameEncodedCb;
    OnAVPacketDecodedCallback m_onPacketDecodedCb;
};


#endif // MEDIACODEC_H
