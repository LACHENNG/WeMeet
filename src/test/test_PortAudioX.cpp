// 引入portaudio头文件
#include "portaudio.h"
// 引入文件操作头文件
#include <fstream>
// 引入异常处理头文件
#include <qDebug>
#include <stdexcept>

#include <src/portaudiox.h>
#include <thread>

// 定义pcm文件的路径
const char* PCM_FILE = "D:/Qt/music_s16.pcm";

// 测试播放音频
int main_testOutput()
{
    PortAudioX audio(PortAudioX::MODE_OUTPUT);

    // 创建一个文件流对象，用于打开pcm文件
    std::ifstream file;
    // 以二进制模式打开pcm文件
    file.open(PCM_FILE, std::ios::binary);

    // 判断是否打开成功
    if (!file.is_open()) {
        // 抛出一个异常，提示无法打开pcm文件
        printf("Can`t open: [%s], use command `ffmpeg -i input.mp3 -f s16le -acodec pcm_s16le -ar 48000 -ac 2 output.pcm` "
               "to generate one and copy to %s\n", PCM_FILE, PCM_FILE);
        throw std::runtime_error("无法打开pcm文件");
    }

    //
    audio.setOnAudioWritableCallback([&file](void* output,  unsigned long frameCount,
                                        size_t nBytesPerFrame, uint8_t n_channel,
                                        const PaStreamCallbackTimeInfo* timeInfo){
        int outputSize = frameCount * nBytesPerFrame * n_channel;
        file.read((char*)output, outputSize);
        if (file.eof()) {
            return paComplete;
        }
        else {
            return paContinue;
        }
    });
    audio.start();

    while(!file.eof()){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // 关闭pcm文件
    file.close();

    // 提示用户播放已经结束
    printf("播放已经结束。\n");
    getchar();
    return 0;
}



// 测试直接获取麦克风音频并直接播放该音频
int main_testRW(){
    PortAudioX audio(PortAudioX::MODE_INPUT_OUTPUT);

    audio.setOnAudioRWCallback([](const void* input, void *output, unsigned long nSampleNum,
                                             size_t nBytesPerSample, uint8_t n_channel,
                                             const PaStreamCallbackTimeInfo* timeInfo){
        memcpy(output, input, nSampleNum * nBytesPerSample * n_channel);
        return paContinue;
    });
    audio.start();

    while(getchar() != 'q'){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // 提示用户播放已经结束
    printf("播放已经结束。\n");
    return 0;
}


// 测试播放pcm
int main(int argc, char* argv[])
{
    if(argc != 2){
        printf("Usage: ./xxx.exe option\n");
        printf("Option Available: [1]. test_play .pcm file\n");
        printf("                  [2]. test capture sound and play to the default device\n");
        getchar();
        return -1;
    }

    if(argv[1][0] == '1'){
        main_testOutput();
    }else if(argv[1][0] == '2'){
        main_testRW();
    }else{
        printf("Unrecogized option: %s (no that you can`t input blanks in option which make it unrecognized\n", argv[1]);
    }

    return 0;
}
