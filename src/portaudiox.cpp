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
    qDebug() << "Input Frame Per Buffer: " << getInputFramesPerBuffer();
    qDebug() << "Output Frame Per Buffer: " << getOutputFramesPerBuffer();
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

unsigned long PortAudioX::getInputFramesPerBuffer(){
    if(kFramesPerBuffer != paFramesPerBufferUnspecified){
        return kFramesPerBuffer;
    }
    const PaStreamInfo* info = Pa_GetStreamInfo(m_audio_io_stream);
    return (info->inputLatency * kSample_rate);
}

unsigned long PortAudioX::getOutputFramesPerBuffer(){
    if(kFramesPerBuffer != paFramesPerBufferUnspecified){
        return kFramesPerBuffer;
    }
    const PaStreamInfo* info = Pa_GetStreamInfo(m_audio_io_stream);
    return (info->outputLatency * kSample_rate);
}

int PortAudioX::getBytesPersample(PaSampleFormat sample_fmt)
{
    switch (sample_fmt){
    case paFloat32:
    case paInt32:
        return 4;
        break;
    case paInt24:
        return 3;
        break;
    case paInt16:
        return 2;
        break;
    case paInt8:
    case paUInt8:
        return 1;
    default:
        return -1;
    }
    return -1;
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
        m_inputParameters->sampleFormat = PA_SAMPEL_FMT; // 使用32位浮点数格式
        m_inputParameters->suggestedLatency = Pa_GetDeviceInfo(m_inputParameters->device)->defaultLowInputLatency; // 使用默认低延迟
        m_inputParameters->hostApiSpecificStreamInfo = NULL; // 不使用特定的API信息
        qDebug() <<"Using Default Audio Input Device: " << Pa_GetDeviceInfo(m_inputParameters->device)->name;
    }
    if(m_mode == MODE_OUTPUT || m_mode == MODE_INPUT_OUTPUT){
        m_outputParameters.reset(new PaStreamParameters);
        m_outputParameters->device = Pa_GetDefaultOutputDevice();
        m_outputParameters->channelCount = kChannel;
        m_outputParameters->sampleFormat = PA_SAMPEL_FMT;
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
        kFramesPerBuffer, // framesPerBuffer, set to 0 to auto adjust
        paNoFlag,         // no flag leads to Interleaved packaging format
        paCallback,       // has to be C-Styel-Function, std::function can`t be cast to C-Style in most cast(unless it stores raw c-style func ptrs)
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
        return ptr->m_onInputCb(input, frameCount, PortAudioX::getBytesPersample(PA_SAMPEL_FMT), ptr->kChannel, timeInfo);
    }
    if(ptr->m_onOutputCb && ptr->m_mode == PortAudioX::MODE_OUTPUT){
        return ptr->m_onOutputCb(output, frameCount, PortAudioX::getBytesPersample(PA_SAMPEL_FMT), ptr->kChannel, timeInfo);
    }
    if(ptr->m_onInputOutputCb && ptr->m_mode == PortAudioX::MODE_INPUT_OUTPUT){
        return ptr->m_onInputOutputCb(input, output, frameCount, PortAudioX::getBytesPersample(PA_SAMPEL_FMT), ptr->kChannel, timeInfo);
    }
    return paComplete; // no user callback provided, stop by default
}
