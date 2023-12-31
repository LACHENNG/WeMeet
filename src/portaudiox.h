﻿// Author: Lance(lang) @ nwpu
// all rights reserved

#ifndef PORTAUDIOX_H
#define PORTAUDIOX_H

#include "portaudio.h"
#include <functional>
#include <exception>
#include <string>
#include <iostream>
#define checkPaErrWithPrompt(c_str_prompt, PaError_err)  \
do{                                                  \
    if (PaError_err != paNoError)                    \
    {                                                \
        std::cout << c_str_prompt + std::string(Pa_GetErrorText(PaError_err)) << std::endl; \
    }                                                \
}while(0)

#define PA_SAMPEL_FMT paInt16

// Open default device for reading audio or writing (paly sound) audio data (PCM data)
// It provides callback mechanism when audio deviced sampled data is ready or
//    a play a frame of sound (writing audio data to speaker) is possible
//
// (By the way, Can you use a std::function as a C style callback
// or,in other words, is it impossible to bind() *this to class member function to make a callback to C API ?
// In most cases you can't.
// except that when you store a C style callback in your std::function, you can use the target() member function.
// The problem is that a C function pointer
// is fundamentally nothing more than an instruction address: "go to this address,
// and execute the instructions you find".
// Any state you want to bring into the function has to either be global, or passed as parameters.
// That is why most C callback APIs have a "context" parameter, typically a void pointer, that you can pass in,
// and just serves to allow you to pass in the data you need.)
class PortAudioX// non-copyable
{
public:
    enum MODE{
        MODE_INPUT,
        MODE_OUTPUT,
        MODE_INPUT_OUTPUT
    };

    static const int kChannel = 2;
    static const int kSample_rate = 48000; // 8kHz(Tel)、16kHz、44.1kHz(CD)、48kHz(DVD)
    static const int kFramesPerBuffer = 960; // 20ms frame , which Opus is support

    using onAudioReadableCallback = std::function<int(const void* input, unsigned long nSampleNum,
                                                       size_t nBytesPerSample, uint8_t n_channel,
                                                       const PaStreamCallbackTimeInfo* timeInfo)>;
    using onAudioWritableCallback = std::function<int(void* output, const unsigned long frameCountLimit,
                                                       size_t nBytesPerFrame, uint8_t n_channel,
                                                       const PaStreamCallbackTimeInfo* timeInfo)>;
    using onAudioRWCallback = std::function<int(const void* input, void *output, unsigned long nSampleNum,
                                                size_t nBytesPerSample, uint8_t n_channel,
                                                const PaStreamCallbackTimeInfo* timeInfo)>;

    // Set mode == MODE_INPUT, only used for capturing audio
    //             MODE_OUTPUT, only used for playing audio
    //             MODE_INPUT_OUTPUT, both
    PortAudioX(MODE mode);
    ~PortAudioX();

    void setOnAudioReadableCallback(const onAudioReadableCallback& cb);;
    void setOnAudioWritableCallback(const onAudioWritableCallback& cb);;
    void setOnAudioRWCallback(const onAudioRWCallback& cb);
    int start();
    int stopUntillDone();
    int stopRightNow();

    // usefull to get nSampleNum in each callback when kFramesPerBuffer is set to paFramesPerBufferUnspecified
    // return 0 if PortAudioX is not initalized using Input mode
    unsigned long getInputFramesPerBuffer();

    // usefull to get nSampleNum in each callback when kFramesPerBuffer is set to paFramesPerBufferUnspecified
    // return 0 if PortAudioX is not initalized using Output mode
    unsigned long getOutputFramesPerBuffer();

    static int getBytesPersample(PaSampleFormat sample_fmt);

private:
    bool init();
    void initParameter();
    // Opens a stream and register rawPaCallback .
    // return false on failure
    bool initOpenStream();

    friend int paCallback(const void* input, void* output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData );
private:
    MODE m_mode;
    onAudioReadableCallback m_onInputCb;
    onAudioWritableCallback m_onOutputCb;
    onAudioRWCallback m_onInputOutputCb;
    // stream for input and output
    PaStream* m_audio_io_stream;
    std::unique_ptr<PaStreamParameters> m_inputParameters;
    std::unique_ptr<PaStreamParameters> m_outputParameters;
};

#endif // PORTAUDIOX_H
