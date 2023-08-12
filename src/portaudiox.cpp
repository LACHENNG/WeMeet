#include <src/portaudiox.h>
#include <qDebug>
#include <stdexcept>

static int paCallback(const void *input, void *output,
                      unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
                      PaStreamCallbackFlags statusFlags, void *userData);

PortAudioX::PortAudioX(MODE mode)
    : m_mode(mode)
{
    init();
}

PortAudioX::~PortAudioX()
{
    Pa_Terminate();
}

void PortAudioX::setOnAudioReadableCallback(const onAudioReadableCallback &cb) {
    if(m_mode == MODE_INPUT || m_mode == MODE_INPUT_OUTPUT){
        m_onInputCb = cb;
    }else{
        qWarning() << "PortAudioX not initalialized using INPUT related mode, callback ignored";
    }
}

void PortAudioX::setOnAudioWritableCallback(const onAudioWritableCallback &cb){
    if(m_mode == MODE_OUTPUT || m_mode == MODE_INPUT_OUTPUT){
        m_onOutputCb = cb;
    }else{
        qWarning() << "PortAudioX not initalialized using OUTPUT related mode, callback ignored";
    }
}

void PortAudioX::setOnAudioRWCallback(const onAudioRWCallback &cb)
{
    if(m_mode == MODE_INPUT_OUTPUT)
        m_onInputOutputCb = cb;
    else qWarning() << "PortAudioX not initalialized using MODE_INPUT_OUTPUT mode, callback ignored";
}

int PortAudioX::start()
{
    auto err = Pa_StartStream(m_audio_io_stream);
    checkPaErrWithPrompt("Pa_StartStream", err);
    return err == paNoError;
}

int PortAudioX::stopUntillDone()
{
    auto err = Pa_StopStream(m_audio_io_stream);
    checkPaErrWithPrompt("Pa_StopStream", err);
    return err == paNoError;
}

int PortAudioX::stopRightNow()
{
    auto err = Pa_CloseStream(m_audio_io_stream);
    checkPaErrWithPrompt("Pa_CloseStream", err);
    return err == paNoError;
}

bool PortAudioX::init()
{
    PaError err = Pa_Initialize();
    checkPaErrWithPrompt("Pa_Initialize", err);
    initParameter();
    return initOpenStream();
}

void PortAudioX::initParameter()
{
    if(m_mode == MODE_INPUT || m_mode == MODE_INPUT_OUTPUT){
        m_inputParameters.reset(new PaStreamParameters);
        m_inputParameters->device = Pa_GetDefaultInputDevice(); // 使用默认输入设备
        m_inputParameters->channelCount = kChannel; // 使用双声道
        m_inputParameters->sampleFormat = paFloat32; // 使用32位浮点数格式
        m_inputParameters->suggestedLatency = Pa_GetDeviceInfo(m_inputParameters->device)->defaultLowInputLatency; // 使用默认低延迟
        m_inputParameters->hostApiSpecificStreamInfo = NULL; // 不使用特定的API信息
        qDebug() <<"Using Default Audio Input Device: " << Pa_GetDeviceInfo(m_inputParameters->device)->name;
    }
    if(m_mode == MODE_OUTPUT || m_mode == MODE_INPUT_OUTPUT){
        m_outputParameters.reset(new PaStreamParameters);
        m_outputParameters->device = Pa_GetDefaultOutputDevice();
        m_outputParameters->channelCount = kChannel;
        m_outputParameters->sampleFormat = paFloat32;
        m_outputParameters->suggestedLatency = Pa_GetDeviceInfo(m_outputParameters->device)->defaultLowOutputLatency;
        m_outputParameters->hostApiSpecificStreamInfo = NULL;
        qDebug() <<"Using Default Audio Output Device: " << Pa_GetDeviceInfo(m_outputParameters->device)->name;
    }
}

bool PortAudioX::initOpenStream()
{
    // open a read and write stream
    auto err = Pa_OpenStream(
        &m_audio_io_stream,
        m_inputParameters.get(),
        m_outputParameters.get(),
        kSample_rate,
        kFrame_per_second, // framesPerBuffer, set to 0 to auto adjust
        paFramesPerBufferUnspecified,  // paClipOff: no clip
        paCallback,  // has to be C-Styel-Function, std::function can`t be cast to C-Style in most cast(unless it stores raw c-style func ptrs)
        this);
    checkPaErrWithPrompt("Pa_OpenStream", err);
    return err == paNoError;
}

static int paCallback(const void *input, void *output, unsigned long frameCount,
                      const PaStreamCallbackTimeInfo *timeInfo,
                      PaStreamCallbackFlags statusFlags, void *userData)
{
    PortAudioX* ptr = (PortAudioX*)userData;
    if(frameCount > 0 && ptr->m_onInputCb && ptr->m_mode == PortAudioX::MODE_INPUT){
        return ptr->m_onInputCb(input, frameCount, sizeof(float), ptr->kChannel, timeInfo);
    }
    if(ptr->m_onOutputCb && ptr->m_mode == PortAudioX::MODE_OUTPUT){
        return ptr->m_onOutputCb(output, frameCount, sizeof(float), ptr->kChannel, timeInfo);
    }
    if(ptr->m_onInputOutputCb && ptr->m_mode == PortAudioX::MODE_INPUT_OUTPUT){
        return ptr->m_onInputOutputCb(input, output, frameCount, sizeof(float), ptr->kChannel, timeInfo);
    }
    return paComplete; // no user callback provided, stop by default
}
