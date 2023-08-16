#include "config.h"
#include "mediacodec.h"
#include "portaudiox.h"
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <opencv2/opencv.hpp>

MediaCodec::MediaCodec(USE_PURPOSE usePurpose) {
    m_usePurpose = usePurpose;
    initVideo();
    initAudio();
}

int32_t MediaCodec::initVideo()
{
    avformat_alloc_output_context2(&m_av_fmt_ctx, nullptr, "h264", nullptr);
    if (!m_av_fmt_ctx) {
        std::cerr << "Can`t create AVFormatContext, avformat_alloc_output_context2 failed" << std::endl;
        return -1;
    }
    // init AVCodec for encoding / decoding h264 format Audio/Video data(AVFrame)
    if(m_usePurpose == USE_AS_ENCODER)
        m_video_codec = avcodec_find_encoder_by_name("libx264");
    else {
        m_video_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    }

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
    // 开启重建帧选项，对视频解码器有效，对音频解码器无效。
    // 重建帧是一种视频解码的优化技术，它可以减少内存的使用和拷贝的开销，但是也会改变输入的数据包
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
    av_opt_set(m_video_codec_ctx->priv_data, "compression_level", "0", 5); // 0表示最快，10表示最慢

    // 打开编码器
    if (avcodec_open2(m_video_codec_ctx, m_video_codec, nullptr) < 0) {
        std::cerr << "avcodec_open2: Error Can`t open encoder" << std::endl;
        return -1;
    }

    // 初始化ffmpeg的AVStream对象，用于存储编码后的视频数据
    m_video_stream = avformat_new_stream(m_av_fmt_ctx, m_video_codec);
    if (!m_video_stream) {
        std::cerr << "无法创建AVStream对象" << std::endl;
        return -1;
    }
    m_video_stream->id = m_av_fmt_ctx->nb_streams - 1; // 设置流的id
    m_video_stream->time_base = m_video_codec_ctx->time_base; // 设置流的时间基准

    int ret = 0;
    if ( (ret = avcodec_parameters_from_context(m_video_stream->codecpar, m_video_codec_ctx)) < 0) {
        std::cerr << avErr2Str("Error on avcodec_parameters_from_context: ", ret);
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

    // 继续初始化ffmpeg的AVFrame对象，用于存储原始视/音频数据
    m_video_frame = av_frame_alloc();
    if (!m_video_frame) {
        std::cerr << "无法创建AVFrame对象" << std::endl;
            return -1;
    }
    m_video_frame->pts = 0; // clear manually, otherwise it is AV_NOPTS_VALUE set by av_frame_alloc by default
        // you can test by assert(AV_NOPTS_VALUE == m_video_frame->pts);


    m_video_frame->format = m_video_codec_ctx->pix_fmt; // 设置帧的像素格式
    m_video_frame->width = m_video_codec_ctx->width; // 设置帧的宽度
    m_video_frame->height = m_video_codec_ctx->height; // 设置帧的高度

    // 分配帧的缓冲区
    int rc = 0;
    if ((rc = av_frame_get_buffer(m_video_frame, 0)) < 0)
    {
        std::cerr << avErr2Str("Error on `av_frame_get_buffer`, reason:", rc);
        return -1;
    }

    // 初始化ffmpeg的AVPacket对象，用于存储编码后的视频数据
    m_av_packet = av_packet_alloc();
    if (!m_av_packet) {
        std::cerr << "无法创建AVPacket对象" << std::endl;
        return -1;
    }
    return 0;
}

int32_t MediaCodec::initAudio()
{
    int ret = 0;

    // init AVCodec for encoding / decoding h264 format Audio/Video data(AVFrame)
    if(m_usePurpose == USE_AS_ENCODER)
        m_audio_codec = avcodec_find_encoder_by_name("libopus");
    else{
        m_audio_codec = avcodec_find_decoder_by_name("libopus");
        // NOTE: the WTF bug stack me for a quit long time
        // do not use avcodec_find_encoder_by_name and avcodec_find_decoder with may dismatch
//         m_audio_codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
    }

    if (!m_audio_codec) {
        std::cerr << "Can`t find Opus encoder/decoder" << std::endl;
        return -1;
    }
    // 初始化ffmpeg的AVCodecContext对象，用于设置编码参数
    m_audio_codec_ctx = avcodec_alloc_context3(m_audio_codec);
    if (!m_audio_codec_ctx) {
        std::cerr << "无法创建AVCodecContext对象" << std::endl;
        return -1;
    }

    // 开启重建帧优化，提高解码速度和质量，减少卡顿等
    if(m_usePurpose == USE_AS_DECODER){
        m_audio_codec_ctx->flags |= AV_CODEC_FLAG_RECON_FRAME;
    }

    m_audio_codec_ctx->frame_size = PortAudioX::kFramesPerBuffer;
    m_audio_codec_ctx->sample_rate = AUDIO_SAMPLE_RATE; // 采样率
    m_audio_codec_ctx->sample_fmt = AUDIO_SAMPLE_FMT; // 采样格式

    // 设置音频编解码器为最快的速度和零延时的模式
    av_opt_set(m_video_codec_ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(m_video_codec_ctx->priv_data, "tune", "zerolatency", 0);

    ret = av_channel_layout_from_mask(&m_audio_codec_ctx->ch_layout, AV_CH_LAYOUT_STEREO);
    if (ret < 0)
        exit(1);
    m_audio_codec_ctx->bit_rate = AUDIO_BIT_RATE;   // 比特率

    // open audio codec
    if((ret = avcodec_open2(m_audio_codec_ctx, m_audio_codec, NULL)) < 0){
        std::cerr << avErr2Str("Error on avcodec_open2: ", ret);
        return -1;
    }
    // avcodec_open2 (可能会改变     m_audio_codec_ctx->sample_fmt 的值，如果编码器不支持的话）
    if(m_audio_codec_ctx->sample_fmt != AUDIO_SAMPLE_FMT){
        qDebug() << " Current Audio codec does not support smaple fmt you specified: " << av_get_sample_fmt_name(AUDIO_SAMPLE_FMT) << "\n"
                 << "The encoding format you specified is forced to change by avcodec_open2:" << av_get_sample_fmt_name( m_audio_codec_ctx->sample_fmt)
                 << "\nAbort";
        exit(1);
    }

    // 初始化ffmpeg的AVStream对象，用于存储编码后的视频数据
    m_audio_stream = avformat_new_stream(m_av_fmt_ctx, m_audio_codec);
    if (!m_audio_stream) {
        std::cerr << "无法创建AVStream对象" << std::endl;
            return -1;
    }
    m_audio_stream->id = m_av_fmt_ctx->nb_streams - 1; // 设置流的id
    m_audio_stream->time_base = m_audio_codec_ctx->time_base; // 设置流的时间基准

    if ( (ret = avcodec_parameters_from_context(m_audio_stream->codecpar, m_audio_codec_ctx)) < 0) {
        std::cerr << avErr2Str("Error on avcodec_parameters_from_context: ", ret);
        return -1;
    }

    // 继续初始化ffmpeg的AVFrame对象，用于存储音频数据
    m_audio_frame = av_frame_alloc();
    if (!m_audio_frame) {
        std::cerr << "无法创建AVFrame对象" << std::endl;
        return -1;
    }
    m_audio_frame->pts = 0; // clear manually, otherwise it is AV_NOPTS_VALUE set by av_frame_alloc by default
                            // you can test by assert(AV_NOPTS_VALUE == m_video_frame->pts);
    // 设置输出帧的参数
    m_audio_frame->nb_samples = m_audio_codec_ctx->frame_size; // 48kHz, libopus do not support 44.1KHz
    m_audio_frame->format = m_audio_codec_ctx->sample_fmt; // 采样格式
    ret = av_channel_layout_copy(&m_audio_frame->ch_layout, &m_audio_codec_ctx->ch_layout);
    if (ret < 0)
        exit(1);

    // 分配帧的缓冲区
    if ((ret = av_frame_get_buffer(m_audio_frame, 0)) < 0) {
        avErr2Str("Error on `av_frame_get_buffer`, reason: [ ", ret);
        return -1;
    }
    return 0;
}

int MediaCodec::encodeAVFrame(cv::Mat mat)
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
        rc = avcodec_receive_packet(m_video_codec_ctx, m_av_packet);
        if(rc == 0){
            if(Config::getInstance().get("showEncoded_video_avpacket_size") == "1"){
                qDebug() << " encoded m_video_packet->size in Bytes : " <<  m_av_packet->size;
            }
            MeetChat::AVPacket av_packet;
            av_packet.set_stream_index(m_video_stream->id); // 设置音视频数据的索引为视频流的id
            av_packet.set_data(m_av_packet->data, m_av_packet->size); // 设置音视频数据的内容
            av_packet.set_size(m_av_packet->size);
            av_packet.set_pts(m_av_packet->pts); // 设置音视频数据的pts
            av_packet.set_dts(m_av_packet->dts); // 设置音视频数据的dts
            av_packet.set_duration(m_av_packet->duration); // 设置音视频数据的duration
            av_packet.set_flags(m_av_packet->flags); // 设置音视频数据的flags
            av_packet.set_pos(m_av_packet->pos); // 设置音视频数据在输入文件中的位置
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

int MediaCodec::encodeAVFrame(const void *audio_src, unsigned long nSampleNum,
                              size_t nBytesPerSample, uint8_t n_channel)
{
    if(m_usePurpose != USE_AS_ENCODER){
        qDebug() << "encodeAVFrame: you can only call encodeAVFrame if and only if "
                    "the MediaCodec is a `USE_AS_ENCODER` " ;
        exit(1);
    }
    /* Important: make sure the frame is writable -- makes a copy if the encoder
         * kept a reference internally */
    int ret = av_frame_make_writable(m_audio_frame);
    if(ret < 0){
        qDebug() << avErr2Str("av_frame_make_writable:", ret);
        exit(1);
    }

    // align audio frame with video frame
    static unsigned long frame_index = 0;
    m_audio_frame->pts = frame_index * m_audio_frame->nb_samples;
    frame_index++;

    if(Config::getInstance().get("showCurrentAudioFramePts") == "1"){
        qDebug() << "set audio pts: " << m_audio_frame->pts;
    }

    // convert PCM to AVFrame
    if(!audioPCM2AVFrame(m_audio_frame, audio_src, nSampleNum, nBytesPerSample, n_channel)){
        qDebug() << "Can`t convert audio to Avframe";
        return -1;
    }

    // send encoder frame

    if ((ret = avcodec_send_frame(m_audio_codec_ctx, m_audio_frame)) < 0) {
        std::cerr << avErr2Str("Error On avcodec_send_frame: ", ret) << std::endl;
        return ret;
    }
    // receive encoded frame
    while(ret >= 0){
        // avcodec_receive_packet 会自动为m_av_packet分配payload，不需要av_new_packet分配内存
        ret = avcodec_receive_packet(m_audio_codec_ctx, m_av_packet);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            break;
        }
        else if(ret < 0){
            qDebug() << avErr2Str("avcodec_receive_packet: ", ret);
            exit(1);
        }

        if(Config::getInstance().get("showEncoded_audio_avpacket_size") == "1"){
            qDebug() << " encoded m_audio_packet->size in Bytes : " <<  m_av_packet->size;
        }

        MeetChat::AVPacket av_packet;
        av_packet.set_stream_index(m_audio_stream->id); // 设置音视频数据的索引为视频流的id
        av_packet.set_data(m_av_packet->data, m_av_packet->size); // 设置音视频数据的内容
        av_packet.set_size(m_av_packet->size);
        av_packet.set_pts(m_audio_frame->pts); // 设置音视频数据的pts
        av_packet.set_dts(m_audio_frame->pts); // 对于音频的dts与pts通常相同
        av_packet.set_duration(m_audio_frame->nb_samples); // 设置音视频数据的duration
        av_packet.set_flags(0); // 设置音视频数据的flags
        av_packet.set_pos(-1); // 设置音视频数据在输入文件中的位

        if(m_onFrameEncodedCb) m_onFrameEncodedCb(av_packet);
        else qDebug() << "m_onFrameEncodedCb not set, discard encoded av_packet";

        // Important: 减少输出数据包的引用计数，并重置其字段, 可以避免内存泄漏和数据混乱的问题
        // 这是因为，如果您不调用 av_packet_unref(pkt) 函数，那么您的类成员里面存储的AVPacket *pkt 将会一直保持对上一次输出数据包的引用，
        // 而不会释放其占用的内存空间。这样可能会导致内存泄漏和数据混乱的问题。
        // 如果引用计数减为0，那么AVPacket对象所指向的内存空间将会被自动释放。这样可以确保每次使用AVPacket对象时，都是一个新的、干净的、可写的对象。
        av_packet_unref(m_av_packet); // 减少输出数据包的引用计数，并重置其字段
    }
    return 0;
}

int MediaCodec::decodeAVPacket(const MeetChat::AVPacket & packet)
{
    assert(m_usePurpose == USE_AS_DECODER);

    // Protobuf AVPacket --> ffmpeg AVPacket
    m_av_packet->stream_index = packet.stream_index();
    m_av_packet->data = (uint8_t*)packet.data().data();
    m_av_packet->size = packet.size();
    m_av_packet->pts = packet.pts();
    m_av_packet->dts = packet.dts();
    m_av_packet->duration = packet.duration();
    m_av_packet->flags = packet.flags();
    m_av_packet->pos = m_av_packet->pos;

    // choose the right AVCodec for decoding audio or video

    AVCodecContext* decoder_ctx = nullptr;
    AVFrame* av_frame = nullptr;

    switch (getAVPacketMediaType(m_av_packet)) {
    case AVMEDIA_TYPE_VIDEO:
        decoder_ctx = m_video_codec_ctx;
        av_frame = m_video_frame;
        break;
    case AVMEDIA_TYPE_AUDIO:
        decoder_ctx = m_audio_codec_ctx;
        av_frame = m_audio_frame;
        break;
    default:
        qDebug() << "Error: unsupport AVPacket type (neither AVMEDIA_TYPE_VIDEO nor AVMEDIA_TYPE_AUDIO)";
        return -1;
        break;
    }

    // decode a single AVPacket
    int ret = 0;
    if ((ret = avcodec_send_packet(decoder_ctx, m_av_packet)) < 0) {
        std::cerr << "can`t send avpacket to decoder, avcodec_send_packet failed with ret = "<< ret  << std::endl;
        return ret;
    }

    while(ret >= 0){
        ret = avcodec_receive_frame(decoder_ctx, av_frame);
        if(ret == 0){
            int data_size = av_get_bytes_per_sample(decoder_ctx->sample_fmt);
            if (data_size < 0) {
                /* This should not occur, checking just for paranoia */
                fprintf(stderr, "Failed to calculate data size\n");
                exit(1);
            }

            // call callback to inform data decoded
            if(m_onPacketDecodedCb) m_onPacketDecodedCb(av_frame);
            else qDebug() << "WARN: m_onPacketDecodedCb not registered, discard decoded data...";
        }else if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            break; // no more output
        }else if(ret < 0){
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

bool MediaCodec::audioPCM2AVFrame(AVFrame *out_avframe, const void *audio_src,
                                  unsigned long frame_count, size_t nBytesPerSample, uint8_t n_channels)
{
    // 检查输入数据的帧数是否与AVFrame的采样数相等
    if (frame_count != out_avframe->nb_samples) {
        qDebug() << "frame_count != out_avframe->nb_samples" << frame_count << "!=" << out_avframe->nb_samples;
        return false;
    }
    // 检查输入数据帧格式使用与AVFrame相同
    assert(AUDIO_SAMPLE_FMT == out_avframe->format);
    if (nBytesPerSample != av_get_bytes_per_sample(AUDIO_SAMPLE_FMT)){
        qDebug() << "nBytesPerSample != sizeof(float)";
        return false;
    }

    if(n_channels != out_avframe->ch_layout.nb_channels){
        qDebug() << "n_channels != out_avframe->ch_layout.nb_channels";
        return false;
    }

    // 拷贝输入数据到AVFrame的缓冲区中
    memcpy(out_avframe->data[0], audio_src, frame_count * n_channels * nBytesPerSample);
    return true;
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

int MediaCodec::checkSampleFmt(const AVCodec *codec, AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

AVMediaType MediaCodec::getAVPacketMediaType(const AVPacket *pkt)
{
    const AVFormatContext *fmt_ctx = m_av_fmt_ctx;

    // 获取该数据包属于哪个流
    int stream_index = pkt->stream_index;

    // 获取该流的信息
    const AVStream *stream = fmt_ctx->streams[stream_index];

    // 获取该流的编解码参数
    const AVCodecParameters *codecpar = stream->codecpar;

    // 获取该流的类型
    enum AVMediaType type = codecpar->codec_type;

    return type;
}

const char *avErr2Str(const char* prompt, int errnum){
    static std::string errStr;
    errStr.clear();
    errStr += prompt;
    if(errnum == 0){
        errStr += " None Error";
    }else if(errnum == AVERROR(EAGAIN)){
        errStr += "input is not accepted in the current state - user must read output with avcodec_receive_packet() (once all output is read, "
               "the packet should be resent, and the call will not fail with EAGAIN).";
    }else if(errnum == AVERROR_EOF){
        errStr += "the encoder has been flushed, and no new frames can be sent to it";
    }else if(errnum == AVERROR(EINVAL)){
        errStr += "codec not opened, it is a decoder, or requires flush";
    }else if(errnum == AVERROR(ENOMEM)){
        errStr += "failed to add packet to internal queue, or similar";
    }else{
        char buf[AV_ERROR_MAX_STRING_SIZE] {0};
        av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, errnum);
        errStr += buf;
    }
    return errStr.c_str();;
}
