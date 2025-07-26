#include <iostream>
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>
extern "C"{
    #include<libavcodec/avcodec.h>
    #include<libavformat/avformat.h>
    #include <libavdevice/avdevice.h>
    #include <libavfilter/avfilter.h>
    #include <libavutil/avutil.h>
    #include<libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
    #include <libswresample/swresample.h>
    #include <libavutil/time.h>
    #include <d3d11.h>
    #include <dxgi.h>
    #include <dxgi1_2.h>
}
using namespace std;

extern AVFormatContext *input_ctx;
extern AVInputFormat *input_fmt;
extern AVCodec *audio_codec;
extern AVCodecContext *audio_codec_ctx;
extern SwrContext *asc;

class AudioQueue{
    public:
        explicit AudioQueue(int _max_size = 1):max_size(_max_size){}
        ~AudioQueue(){
            while (!packets.empty()) {
                AVPacket* pkt = packets.front();
                av_packet_free(&pkt);
                packets.pop();
            }
        }
        void push(AVPacket* packet);
        bool pop(AVPacket* packet);

    private:
        queue<AVPacket*> packets;
        std::mutex mutex;
        condition_variable not_full;
        condition_variable not_empty;
        size_t max_size;
};

extern AudioQueue *audio_packets;

void init_device();

void init_audio_encoder();

int init_swr();

void audio_encode();
