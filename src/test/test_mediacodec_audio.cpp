/*************************************************************************
*    File Name: test_mediacodec_audio.cpp
*    Description: This test code test sample audio using PortAudioX
                  and decode it using Opus, then decode it and play
                  You can use your phone to play audio out loud and plug in headphones
                  on your computer to test if the encoding and decoding are normal.

*    Version: V1.0
*    Author: Lance @ npwu
*    Email: transplus@foxmail.com
*    Create Time: 2023-08-16
*    License: GPL license, and For learning use only, not for commercial use.
*    Other Info: None
*    Modification Info: None
*************************************************************************/

#include "portaudio.h"

#include <fstream>
#include <qDebug>
#include <stdexcept>
#include <string>

#include <src/mediacodec.h>
#include <src/portaudiox.h>
#include <thread>
#include <mutex>

std::mutex pcm_mutex;
std::vector<uint8_t> pcm_data; // 播放队列(音频队列）


// get audio and play directly
// without encoding, this test the PortAudioX
// before test the MediaCodec
int main_test_audio_direct_play(){

    PortAudioX audioInput(PortAudioX::MODE_INPUT);
    PortAudioX audioOutput(PortAudioX::MODE_OUTPUT);

    // 采集音频并发送到编码器
    audioInput.setOnAudioReadableCallback([](const void* input, unsigned long nSampleNum,
                                             size_t nBytesPerSample, uint8_t n_channel,
                                             const PaStreamCallbackTimeInfo* timeInfo){
        // qDebug()<<"Raw PCM data size: " << nSampleNum * nBytesPerSample * n_channel << " B";
        int nBytes = nSampleNum *n_channel * nBytesPerSample;

        {
            std::lock_guard<std::mutex> lock(pcm_mutex);
            assert(nBytes > 0);
            std::copy((char*)input, (char*)input + nBytes, std::back_inserter(pcm_data));
        }

        return paContinue;
    });

    // 设置播放回调函数，一直从播放队列获取数据播放
    audioOutput.setOnAudioWritableCallback([](void* output, const unsigned long frameCountLimit,
                                              size_t nBytesPerFrame, uint8_t n_channel,
                                              const PaStreamCallbackTimeInfo* timeInfo){
        // reduce lock contention
        std::vector<uint8_t> local_pcm;
        {
            std::lock_guard<std::mutex> lock(pcm_mutex);
            std::swap(local_pcm, pcm_data);
        }

        // copy at most nBytes to output buffer
        int nMaxBytes = std::min(frameCountLimit * nBytesPerFrame * n_channel, local_pcm.size());
        // qDebug() << "Paly " << nMaxBytes << " Bytes";
        if(nMaxBytes > 0) std::copy(local_pcm.begin(), local_pcm.begin() + nMaxBytes, (uint8_t*)output);

        return paContinue;
    });

    audioInput.start();
    audioOutput.start();

    getchar();
    return 0;
}


// This test try to get audio input from default device
// and encode it ,
// Then decode it and play to test the functionality of MediaCodec
int main_test_audio_codec_play(){
    qDebug() << "test media codec...";
    PortAudioX audioInput(PortAudioX::MODE_INPUT);
    PortAudioX audioOutput(PortAudioX::MODE_OUTPUT);

    MediaCodec audioEnc(MediaCodec::USE_AS_ENCODER);
    MediaCodec audioDec(MediaCodec::USE_AS_DECODER);

    audioInput.setOnAudioReadableCallback([&audioEnc](const void* input, unsigned long nSampleNum,
                                                      size_t nBytesPerSample, uint8_t n_channel,
                                                      const PaStreamCallbackTimeInfo* timeInfo) {
        // std::cout << "audio readable callback at thread : " << std::this_thread::get_id() << std::endl;
        int nBytes = nSampleNum *n_channel * nBytesPerSample;
        audioEnc.encodeAVFrame(input, nSampleNum, nBytesPerSample, n_channel);
        return paContinue;
    });

    // 接收编码结果并解码
    audioEnc.setOnFrameEncodedCallback([&audioDec](const MeetChat::AVPacket& packet){
        audioDec.decodeAVPacket(packet);
    });

    // 接受解码结果并添加到播放队列
    audioDec.setOnPacketDecodedCallback([](const AVFrame* audio_frame){
        int nBytes = audio_frame->nb_samples *
                     audio_frame->ch_layout.nb_channels *
                     av_get_bytes_per_sample((AVSampleFormat)audio_frame->format);
        const uint8_t* start = (uint8_t*)audio_frame->data[0];

        {
            std::lock_guard<std::mutex> lock(pcm_mutex);
            std::copy(start, start + nBytes, std::back_inserter(pcm_data));
        }
    });

    // 设置播放回调函数，一直从播放队列获取数据播放
    audioOutput.setOnAudioWritableCallback([](void* output, const unsigned long frameCountLimit,
                                              size_t nBytesPerFrame, uint8_t n_channel,
                                              const PaStreamCallbackTimeInfo* timeInfo){
        // std::cout << "audio writeable callback at thread : " << std::this_thread::get_id() << std::endl;
        std::vector<uint8_t> local_pcm;
        {
            std::lock_guard<std::mutex> lock(pcm_mutex);
            std::swap(local_pcm, pcm_data);
        }
        std::copy(local_pcm.begin(), local_pcm.end(), (uint8_t*)output);
        return paContinue;
    });

    audioInput.start();
    audioOutput.start();

    getchar();
    return 0;
}

int main(){
    int opt = 1;
    if(opt == 1)
        main_test_audio_codec_play();
    else
        main_test_audio_direct_play();
}
