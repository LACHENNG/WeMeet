#include "audioplayer.h"
#include "portaudiox.h"

#include <assert.h>
#include <qDebug>

AudioPlayer::AudioPlayer()
  : m_state(STATE_NOT_INITED),
    m_audio_out(new PortAudioX(PortAudioX::MODE_OUTPUT))
{
    init();
}

AudioPlayer::~AudioPlayer()
{
    // default dtor is okay
}

void AudioPlayer::queueToPlay(void *audio_src, int src_bytes)
{
    std::lock_guard<std::mutex> lock(pcm_mutex);
    assert(src_bytes > 0);
    std::copy((char*)audio_src, (char*)audio_src + src_bytes, std::back_inserter(pcm_data));
}

void AudioPlayer::init()
{
    // allready inited, skip
    if(m_state != STATE_NOT_INITED){
        return ;
    }

    m_audio_out->setOnAudioWritableCallback([this](void* output, const unsigned long frameCountLimit,
                                                   size_t nBytesPerFrame, uint8_t n_channel,
                                                   const PaStreamCallbackTimeInfo* timeInfo){
        // you can only paly nBytesLimit
        int nBytesLimit = frameCountLimit * nBytesPerFrame * n_channel;

        {
            std::lock_guard<std::mutex> lock(pcm_mutex);

            if(pcm_data.size() < nBytesLimit){
                return paContinue;
            }
            else{
                std::copy(pcm_data.begin(), pcm_data.begin() + nBytesLimit, (uint8_t*)output);
                pcm_data.erase(pcm_data.begin(), pcm_data.begin() + nBytesLimit);
            }
        }

        return paContinue;
    });

    m_state = STATE_INITED;
}

bool AudioPlayer::start()
{
    if(m_state != STATE_INITED){
        return false;
    }
    // clear ouput buffer
    pcm_data.clear();
    if(m_audio_out->start()){
        m_state = STATE_STARTED;
        return true;
    }
    // can`t start m_audio_out
    return false;
}

bool AudioPlayer::stop()
{
    if(m_state != STATE_STARTED){
        return false;
    }

    if( m_audio_out->stopRightNow() ){
        m_state = STATE_STOPPED;
        return true;
    }
    // can`t stop m_audio_out
    return false;
}
