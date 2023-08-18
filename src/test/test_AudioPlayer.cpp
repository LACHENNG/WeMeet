
#include <fstream>
#include <src/audioplayer.h>
#include <qDebug>
#include <thread>
#include <iostream>

const char* PCM_FILE = "D:/Qt/music_s16.pcm";

// this test program reads pcm audio data
// and using AudioPlayer to play it
int main_testOutput()
{
    AudioPlayer player;
    if(!player.start()){
        std::cout << "can`t start AudioPlayer" << std::endl;
        exit(1);
    }
    std::ifstream file;
    file.open(PCM_FILE, std::ios::binary);
    if (!file.is_open()) {
        printf("Can`t open: [%s], use command `ffmpeg -i input.mp3 -f s16le -acodec pcm_s16le -ar 48000 -ac 2 output.pcm` "
               "to generate one and copy to %s\n", PCM_FILE, PCM_FILE);
        throw std::runtime_error("Can`t open pcm file");
    }

    std::vector<uint8_t> buffer(960*2*2, 0);
    while(!file.eof()){
        file.read((char*)buffer.data(), buffer.size());
        player.queueToPlay(buffer.data(), file.gcount());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    file.close();
    player.stop();

    printf("Audio Queue Playing Done\n");
    getchar();
    return 0;
}


// test play pcm
int main(int argc, char* argv[])
{
    main_testOutput();
    return 0;
}
