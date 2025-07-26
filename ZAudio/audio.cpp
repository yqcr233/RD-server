#include "audio.hpp"

AVFormatContext *input_ctx = nullptr;
AVInputFormat *input_fmt = nullptr;
AVCodec *audio_codec = nullptr;
AVCodecContext *audio_codec_ctx = nullptr;
AudioQueue *audio_packets = nullptr;
SwrContext *asc = NULL;

void init_device(){
    const char* device_name = "audio=阵列麦克风 (AMD Audio Device)";
    input_fmt = (AVInputFormat *)av_find_input_format("dshow");

    avformat_open_input(&input_ctx, device_name, input_fmt, NULL);
}

void init_audio_encoder(){
    audio_codec = (AVCodec *)avcodec_find_encoder(AV_CODEC_ID_AAC); // AAC编码
    audio_codec_ctx = avcodec_alloc_context3(audio_codec);

    // 设置编码参数
    audio_codec_ctx->bit_rate = 128000;
    audio_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP; // 浮点型PCM
    audio_codec_ctx->sample_rate = 44100;
    // 设置声道布局
    AVChannelLayout layout = {};
    // 假设我们要设置成立体声
    av_channel_layout_default(&layout, AV_CH_LAYOUT_STEREO);  // 第二个参数是声道数（例如2表示立体声）
    audio_codec_ctx->ch_layout = layout;

    avcodec_open2(audio_codec_ctx, audio_codec, NULL);
}

int init_swr(){
    ///2 音频重采样 上下文初始化
    swr_alloc_set_opts2(&asc, 
                    &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO, // out_ch_layout
                    AV_SAMPLE_FMT_FLTP,    // out_sample_fmt
                    44100,                // out_sample_rate
                    &(AVChannelLayout)AV_CHANNEL_LAYOUT_5POINT1, // in_ch_layout
                    AV_SAMPLE_FMT_S16,   // in_sample_fmt
                    44100,                // in_sample_rate
                    0,                    // log_offset
                    NULL);                // log_ctx
    if (!asc) {
        av_log(NULL, AV_LOG_ERROR, "%s", "swr_alloc_set_opts failed!");
        return -1;
    }
    int ret = swr_init(asc);
    if (ret < 0) {
        fprintf(stdout, "swr_init error\n");
        return ret;
    }
}

void audio_encode(){
    init_device();
    init_audio_encoder();

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    frame->format = audio_codec_ctx->sample_fmt;
    frame->ch_layout = audio_codec_ctx->ch_layout;
    frame->nb_samples = audio_codec_ctx->frame_size;

    // while(true) {
    //     int ret = av_read_frame(&input_ctx, )
    // }

}