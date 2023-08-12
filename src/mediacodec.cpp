#include "config.h"
#include "mediacodec.h"
#include "qimage.h"
#include "ui/chatwindow.h"
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <opencv2/opencv.hpp>

int32_t MediaCodec::init()
{
    avformat_alloc_output_context2(&m_video_fmt_ctx, nullptr, "h264", nullptr);
    if (!m_video_fmt_ctx) {
        std::cerr << "Can`t create AVFormatContext, avformat_alloc_output_context2 failed" << std::endl;
        return -1;
    }
    // init AVCodec for encoding / decoding h264 format Audio/Video data(AVFrame)
    if(m_usePurpose == USE_AS_ENCODER)
        m_video_codec = avcodec_find_encoder_by_name("libx264");
    else m_video_codec = avcodec_find_decoder(AV_CODEC_ID_H264);

    if (!m_video_codec) {
        std::cerr << "Can`t find libx264 encoder/decoder" << std::endl;
        return -1;
    }

    // 初始化ffmpeg的AVCodecContext对象，用于设置编码参数
    m_video_codec_ctx = avcodec_alloc_context3(m_video_codec);
    if (!m_video_codec_ctx) {
        std::cerr << "无法创建AVCodecContext对象" << std::endl;
            return -1;
    }
    if(m_usePurpose == USE_AS_DECODER){
        m_video_codec_ctx->flags |= AV_CODEC_FLAG_RECON_FRAME;
    }

    m_video_codec_ctx->width = VIDEO_WIDTH; // 设置视频宽度
    m_video_codec_ctx->height = VIDEO_HEIGHT; // 设置视频高度
    m_video_codec_ctx->time_base = {1, VIDEO_FPS}; // 设置视频时间基准
    m_video_codec_ctx->framerate = {VIDEO_FPS, 1}; // 设置视频帧率
    m_video_codec_ctx->bit_rate = VIDEO_BIT_RATE; // 设置视频比特率
    m_video_codec_ctx->gop_size = VIDEO_GOP_SIZE; // 设置视频关键帧间隔
    m_video_codec_ctx->has_b_frames = 0;
    m_video_codec_ctx->max_b_frames = 0;          // 设置视频最大B帧数
    m_video_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; // 设置视频像素格式
    m_video_codec_ctx->rc_buffer_size = VIDEO_BIT_RATE;

    // 设置编码器为最快的速度和零延时的模式
    av_opt_set(m_video_codec_ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(m_video_codec_ctx->priv_data, "tune", "zerolatency", 0);


    // 打开编码器
    if (avcodec_open2(m_video_codec_ctx, m_video_codec, nullptr) < 0) {
        std::cerr << "无法打开编码器" << std::endl;
            return -1;
    }

    // 初始化ffmpeg的AVStream对象，用于存储编码后的视频数据
    m_video_stream = avformat_new_stream(m_video_fmt_ctx, m_video_codec);
    if (!m_video_stream) {
        std::cerr << "无法创建AVStream对象" << std::endl;
            return -1;
    }
    m_video_stream->id = m_video_fmt_ctx->nb_streams - 1; // 设置流的id
    m_video_stream->time_base = m_video_codec_ctx->time_base; // 设置流的时间基准

    // 复制编码器参数到流参数
    if (avcodec_parameters_from_context(m_video_stream->codecpar, m_video_codec_ctx) < 0) {
        std::cerr << "无法复制编码器参数到流参数" << std::endl;
        return -1;
    }

    // 初始化ffmpeg的SwsContext对象，用于转换图像格式
    if(m_usePurpose == USE_AS_ENCODER){
        m_sws_ctx = sws_getContext(VIDEO_WIDTH, VIDEO_HEIGHT,
                                   AV_PIX_FMT_BGR24, VIDEO_WIDTH, VIDEO_HEIGHT,
                                   AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
    }
    else if(m_usePurpose == USE_AS_DECODER){
        m_sws_ctx = sws_getContext(VIDEO_WIDTH, VIDEO_HEIGHT,
                                   AV_PIX_FMT_YUV420P, VIDEO_WIDTH, VIDEO_HEIGHT,
                                   AV_PIX_FMT_BGR24, SWS_BICUBIC, nullptr, nullptr, nullptr);
    }
    if (!m_sws_ctx) {
        std::cerr << "无法创建SwsContext对象" << std::endl;
            return -1;
    }

    // 继续初始化ffmpeg的AVFrame对象，用于存储原始图像数据
    m_video_frame = av_frame_alloc();


    m_video_frame->pts = 0; // clear manually, otherwise it is AV_NOPTS_VALUE set by av_frame_alloc by default
                            // you can test by assert(AV_NOPTS_VALUE == m_video_frame->pts);
    if (!m_video_frame) {
        std::cerr << "无法创建AVFrame对象" << std::endl;
            return -1;
    }
    m_video_frame->format = m_video_codec_ctx->pix_fmt; // 设置帧的像素格式
    m_video_frame->width = m_video_codec_ctx->width; // 设置帧的宽度
    m_video_frame->height = m_video_codec_ctx->height; // 设置帧的高度


    // 分配帧的缓冲区
    if (av_frame_get_buffer(m_video_frame, 0) < 0)
    {
        std::cerr << "无法分配帧的缓冲区" << std::endl;
        return -1;
    }


    // 初始化ffmpeg的AVPacket对象，用于存储编码后的视频数据
    m_video_packet = av_packet_alloc();
    if (!m_video_packet) {
        std::cerr << "无法创建AVPacket对象" << std::endl;
            return -1;
    }
    return 0;
}

int MediaCodec::encodeFrame(cv::Mat mat)
{
    assert(m_usePurpose == USE_AS_ENCODER);
    if(mat.empty() || mat.rows != VIDEO_HEIGHT || mat.cols != VIDEO_WIDTH){
        return -1;
    }

    // 将opencv的Mat对象转换为ffmpeg的AVFrame对象
    cvMat2AVFrame(mat, m_video_frame, m_sws_ctx);

    // 设置帧的pts
    if(Config::getInstance().get("showCurrentVideoFramePts") == "1"){
        qDebug() << "set pts: " << m_video_frame->pts;
    }
    m_video_frame->pts += av_rescale_q(1, m_video_codec_ctx->time_base, m_video_stream->time_base);

    // 编码一帧图像
    int rc = 0;
    if ((rc = avcodec_send_frame(m_video_codec_ctx, m_video_frame)) < 0) {
        std::cerr << "无法发送帧到编码器" << std::endl;
        return rc;
    }
    while(rc >= 0){
        rc = avcodec_receive_packet(m_video_codec_ctx, m_video_packet);
        if(rc == 0){
            if(Config::getInstance().get("showEncoded_video_avpacket_size") == "1"){
                qDebug() << " encoded m_video_packet->size in Bytes : " <<  m_video_packet->size;
            }
            MeetChat::AVPacket av_packet;
            av_packet.set_stream_index(m_video_stream->id); // 设置音视频数据的索引为视频流的id
            av_packet.set_data(m_video_packet->data, m_video_packet->size); // 设置音视频数据的内容
            av_packet.set_size(m_video_packet->size);
            av_packet.set_pts(m_video_packet->pts); // 设置音视频数据的pts
            av_packet.set_dts(m_video_packet->dts); // 设置音视频数据的dts
            av_packet.set_duration(m_video_packet->duration); // 设置音视频数据的duration
            av_packet.set_flags(m_video_packet->flags); // 设置音视频数据的flags
            av_packet.set_pos(m_video_packet->pos); // 设置音视频数据在输入文件中的位置
            m_onFrameEncodedCb(av_packet);
        }else if(rc == AVERROR(EAGAIN) || rc == AVERROR_EOF){
            break; // no more output
        }else if(rc < 0){
            std::cerr << "AVERROR(EINVAL) codec not opened, or it is a decoder" << std::endl;
            return -1;
        }
    }
    return 0;
}

int MediaCodec::decodeAVPacket(const MeetChat::AVPacket & packet)
{
    assert(m_usePurpose == USE_AS_DECODER);

    // Protobuf AVPacket --> ffmpeg AVPacket
    m_video_packet->stream_index = packet.stream_index();
    m_video_packet->data = (uint8_t*)packet.data().c_str();
    m_video_packet->size = packet.data().size();
    m_video_packet->pts = packet.pts();
    m_video_packet->dts = packet.dts();
    m_video_packet->duration = packet.duration();
    m_video_packet->flags = packet.flags();
    m_video_packet->pos = m_video_packet->pos;

    // decode a single AVPacket
    int rc = 0;
    if ((rc = avcodec_send_packet(m_video_codec_ctx, m_video_packet)) < 0) {
        std::cerr << "can`t send avpacket to decoder, avcodec_send_packet failed with rc = "<< rc  << std::endl;
            std::cerr <<    AVERROR(EAGAIN) << " " << AVERROR_EOF << AVERROR(EINVAL);
        return rc;
    }

    while(rc >= 0){
        rc = avcodec_receive_frame(m_video_codec_ctx, m_video_frame);
        if(rc == 0){
            // call callback to inform data decoded
            if(m_onPacketDecodedCb) m_onPacketDecodedCb(m_video_frame);
            else qDebug() << "WARN: m_onPacketDecodedCb not registered, discard decoded data...";
        }else if(rc == AVERROR(EAGAIN) || rc == AVERROR_EOF){
            break; // no more output
        }else if(rc < 0){
            std::cerr << "AVERROR(EINVAL) codec not opened, or it is a decoder" << std::endl;
            return -1;
        }
    }
    return 0;
}

void MediaCodec::cvMat2AVFrame(const cv::Mat &inMat, AVFrame *out_video_frame, SwsContext *sws_ctx){
    uint8_t* data[AV_NUM_DATA_POINTERS] = {0};
    int linesize[AV_NUM_DATA_POINTERS] = {0};
    data[0] = inMat.data;
    linesize[0] = inMat.cols * inMat.channels();
    // make a default SwsContext, converting BGR24-->YUV420p
    if(sws_ctx == nullptr){
        AVPixelFormat input_pix_fmt = (AVPixelFormat)out_video_frame->format;
        cv::Mat output_mat(VIDEO_HEIGHT, VIDEO_WIDTH, CV_8UC3);
        sws_ctx = sws_getContext(inMat.cols, inMat.rows, AV_PIX_FMT_BGR24,
                                             out_video_frame->width, out_video_frame->height, input_pix_fmt,
                                             SWS_BICUBIC, nullptr, nullptr, nullptr);
    }
    sws_scale(sws_ctx, data, linesize, 0, inMat.rows, out_video_frame->data, out_video_frame->linesize);
}

void MediaCodec::avFrame2cvMat(const cv::Mat &out_mat, AVFrame *in_frame, SwsContext *sws_ctx)
{
    AVPixelFormat input_pix_fmt = (AVPixelFormat)in_frame->format;
//    cv::Mat output_mat(VIDEO_HEIGHT, VIDEO_WIDTH, CV_8UC3);
    if(sws_ctx == nullptr){
        sws_ctx = sws_getContext(in_frame->width, in_frame->height, input_pix_fmt,
                             out_mat.cols, out_mat.rows, AV_PIX_FMT_BGR24,
                             SWS_BICUBIC, nullptr, nullptr, nullptr);
    }

    uint8_t* data[AV_NUM_DATA_POINTERS] = {0};
    int linesize[AV_NUM_DATA_POINTERS] = {0};
    linesize[0] = out_mat.cols * out_mat.channels();
    data[0] = out_mat.data;
    sws_scale(sws_ctx, in_frame->data, in_frame->linesize, 0, in_frame->height, data, linesize);
}
