/*************************************************************************
*    File Name: audioplayer.h
*    Description: This class is basically a wrapper to PortAudioX(MODE_OUTPUT)
*                 But provide a simple async interface to simplify the routine
*                 to play audio
*    Author: Lance @ npwu
*    Email: transplus@foxmail.com
*    Create Time: 2023-08-17
*    License: GPL license, and For learning use only, not for commercial use.
*    Other Info: None
*    Modification Info: None
*************************************************************************/
#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H


#include <deque>
#include <memory>
#include <mutex>

class PortAudioX;

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer(); // force outline

    // MT-Safe
    // Queue audio to audio playing queue
    // FIXME: if the client invoke queueToPlay too fast, the internal buffer may consume to much memory
    void queueToPlay(void *audio_src, int src_bytes);

    bool start();

    bool stop();

private:
    void init();

    enum INTERNAL_STATE{
        STATE_NOT_INITED,
        STATE_INITED,
        STATE_STARTED,
        STATE_STOPPED,
    };
    // interanl state machine to prevent public interface misuse
    INTERNAL_STATE m_state = STATE_NOT_INITED;

    std::mutex pcm_mutex;          // protects pcm_data
    std::deque<uint8_t> pcm_data; // audio paly queue

    std::unique_ptr<PortAudioX> m_audio_out;
};

#endif // AUDIOPLAYER_H
