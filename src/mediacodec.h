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
#define VIDEO_GOP_SIZE 30        // video key frame(I frame) interval, gap(#frames) between two I frames
#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_CHANNELS 2
#define AUDIO_BIT_RATE 128000
#define AUDIO_FRAME_SIZE PortAudioX::kFramesPerBuffer
#define AUDIO_SAMPLE_FMT AV_SAMPLE_FMT_S16

// WARN: not MT-safe
//  return a string containing an error string
//  corresponding to the AVERROR code errnum.
const char *avErr2Str(const char* prompt, int errnum);

// This MediaCodec encodes/decodes video and sounds
// to and from MeetChat::VideoFrame
// (see protoc/message.proto for more details of how this protobuf message is defined)
//
// It use H.264 to encode video frames
//    use Opus[ /ˈoʊpəs/，欧普斯 ] to encode sounds
//
// Important: Do not use a single MediaCodec as a encoder and decoder at the same time
//            if you need encode and decode, make two seperate MediaCodec instance to do that
//            with different construct paramters.
//            But, you can encode audio and video or decoded audio and video within a single MediaCodec
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

    MediaCodec(USE_PURPOSE usePurpose);

    // FIXME: release resources in dtor
    ~MediaCodec() {}

    // callback on data encoded
    void setOnFrameEncodedCallback(const OnAVFrameEncodedCallback& cb) { m_onFrameEncodedCb = cb; }
    void setOnPacketDecodedCallback(const OnAVPacketDecodedCallback& cb){ m_onPacketDecodedCb = cb; }

    // encode one Frame
    // Note: setOnFrameEncodedCallback to received encoded data
    int encodeAVFrame(cv::Mat mat);

    // encode multi audio frames
    // Note: setOnFrameEncodedCallback to received encoded data
    int encodeAVFrame(const void* audio, unsigned long nSampleNum,
                    size_t nBytesPerSample, uint8_t n_channel);

    // decode one MeetChat::Message which must has type == Av, which stores a MeetChat::AVPacket ( encoded by encodeFrame)
    // Note: setOnPacketDecodedCallback to received decoded data
    int decodeAVPacket(const MeetChat::AVPacket& av_packet);

    // convert cv::Mat to AvFrame using given SwsContext
    // if sws_ctx is left null, cv::Mat(assume BGR24 format) is convert to AVFrame (YUV420 format)
    static void cvMat2AVFrame(const cv::Mat& inMat, AVFrame* out_video_frame, SwsContext* sws_ctx = nullptr);

    // convert audio (PCM data) to AvFrame
    static bool audioPCM2AVFrame(AVFrame* out_audio_frame, const void* audio_src,
                                 unsigned long nSampleNum, size_t nBytesPerSample, uint8_t n_channels);

    // convert AvFrame to cv::Mat using given SwsContext
    // if sws_ctx is left null, AVFrame (assume YUV420 format) is convert to cv::Mat(BGR24)
    static void avFrame2cvMat(const cv::Mat& out_Mat, AVFrame* in_frame, SwsContext* sws_ctx = nullptr);

    /* check that a given sample format is supported by the encoder */
    static int checkSampleFmt(const AVCodec *codec, enum AVSampleFormat sample_fmt);

    AVMediaType getAVPacketMediaType(const AVPacket *pkt);

    // get***(const AVFrame* audio_frame): convenient function to get nb_sample, nb_channle, bytes_per_sample of audio AVFrame
    static int getNumberSample(const AVFrame* audio_frame) { return audio_frame->nb_samples; }
    static int getNumberChannel(const AVFrame* audio_frame) { return audio_frame->ch_layout.nb_channels; }
    static int getBytesPerSample(const AVFrame* audio_frame) { return av_get_bytes_per_sample((AVSampleFormat)audio_frame->format);}
    static int getTotalBytesIn(const AVFrame* audio_frame){ return getNumberSample(audio_frame)*getNumberChannel(audio_frame)*getBytesPerSample(audio_frame); }

private:
    // return 0 if sucess, otherwise a negative error code is returnd;
    int32_t initVideo();
    int32_t initAudio();

    USE_PURPOSE m_usePurpose;

    // ffmpeg的AVFormatContext对象，AVFormatContext结构体可以包含多个流的信息, 不用为音视频流各创建一个，公用即可
    AVFormatContext* m_av_fmt_ctx = nullptr;
    // ffmpeg的AVCodec对象，用于编码h264格式的视频数据
    const AVCodec* m_video_codec = nullptr;
    const AVCodec* m_audio_codec = nullptr;
    // 初始化ffmpeg的AVCodecContext对象，用于设置编码参数
    AVCodecContext* m_video_codec_ctx = nullptr;
    AVCodecContext* m_audio_codec_ctx = nullptr;

    // 初始化ffmpeg的AVStream对象，用于存储编码后的视/音频数据
    AVStream* m_video_stream = nullptr;
    AVStream* m_audio_stream = nullptr;

    // 初始化ffmpeg的SwsContext对象，用于转换图像格式
    SwsContext* m_sws_ctx = nullptr;

    // 初始化ffmpeg的AVFrame对象，用于存储原始视/音频数据
    AVFrame* m_video_frame = nullptr;
    AVFrame* m_audio_frame = nullptr;

    // 初始化ffmpeg的AVPacket对象，用于存储编码后的视频数据
    AVPacket* m_av_packet = nullptr;

    // 初始化protobuf的VideoData对象，用于序列化视频数据
    OnAVFrameEncodedCallback m_onFrameEncodedCb;
    OnAVPacketDecodedCallback m_onPacketDecodedCb;
};

#endif // MEDIACODEC_H
