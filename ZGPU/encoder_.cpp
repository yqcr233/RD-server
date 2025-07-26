#include "encoder_.hpp"

GPUEncoder::GPUEncoder() : hw_device_ctx(nullptr, hwDeviceCtx_deleter){}

void GPUEncoder::initEncode(int dst_width, int dst_height, int bitrate, int fps){
    codec = avcodec_find_encoder_by_name("hevc_amf");
    if(!codec) {
        cerr<< "not found codec"<<endl;
    }
    
    codec_ctx = shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec), [](AVCodecContext* temp){
        if(temp) {
            avcodec_close(temp);
            if(temp) avcodec_free_context(&temp);
        }
    }); 
    if(!codec_ctx){
        cerr<<"fail to allocate codec context"<<endl;
    }

    // 根据屏幕数据据配置编码参数并打开编码器
    codec_ctx->width = dst_width; 
    codec_ctx->height = dst_height;

    codec_ctx->time_base = {1, 90000}; 
    codec_ctx->framerate = {60, 1}; // 不设帧率宏块过多会花屏
    codec_ctx->bit_rate = bitrate;
    codec_ctx->gop_size = 3600;
    codec_ctx->max_b_frames = 0;
    codec_ctx->pix_fmt = AV_PIX_FMT_D3D11; // 设置输入帧数据为d3d11纹理输入

    AVDictionary* options_ = nullptr;

    int mode = 1;
    av_dict_set(&options_, "quality", "speed", 0);  // 可选: speed, balanced, quality
    av_dict_set(&options_, "usage", "ultralowlatency", 0);

    AVBufferRef* hw_device_ctx_ = nullptr;
    int err = av_hwdevice_ctx_create(&hw_device_ctx_, AV_HWDEVICE_TYPE_D3D11VA, NULL, NULL, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to create a device. Error code: %s\n", av_err2str(err));
    }
    hw_device_ctx.reset(hw_device_ctx_);

    // 获取硬件设备及其上下文
    AVHWDeviceContext* hw_ctx = (AVHWDeviceContext*)hw_device_ctx_->data;
    AVD3D11VADeviceContext* hw_d3d_ctx = (AVD3D11VADeviceContext*)hw_ctx->hwctx;
    d3d_device = hw_d3d_ctx->device;
    d3d_context = hw_d3d_ctx->device_context;

    // 设置编码器上下文帧缓冲区
    unique_ptr<AVBufferRef, decltype(hwDeviceCtx_deleter)> hw_frames_ref(nullptr, hwDeviceCtx_deleter);
    hw_frames_ref.reset(av_hwframe_ctx_alloc(hw_device_ctx.get()));
    AVBufferRef* hw_frames_ref_ = hw_frames_ref.get();
    if(!hw_frames_ref_) {
        fprintf(stderr, "Failed to create VAAPI frame context.\n");
    }
    AVHWFramesContext* frames_ctx = nullptr;
    frames_ctx = (AVHWFramesContext*)hw_frames_ref->data;
    auto d3d11Ctx = (AVD3D11VAFramesContext*)frames_ctx->hwctx;
    d3d11Ctx->BindFlags |= D3D11_BIND_RENDER_TARGET;
    frames_ctx->format = AV_PIX_FMT_D3D11;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width = dst_width;
    frames_ctx->height = dst_height;
    frames_ctx->initial_pool_size = 10;
    if((err = av_hwframe_ctx_init(hw_frames_ref.get())) < 0) {
        fprintf(stderr, "Failed to initialize frame context."
                "Error code: %s\n",av_err2str(err));
        // av_buffer_unref(&hw_frames_ref_);
        return;
    }
    codec_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref.get());
    if(!codec_ctx->hw_frames_ctx) {
        fprintf(stderr, "fail to create ctx->hw_frames_ctx");
    }
    if(avcodec_open2(codec_ctx.get(), codec, &options_)<0){
        fprintf(stderr, "fail ot open codec"); 
    } 

    av_dict_free(&options_);
}

void GPUEncoder::encode(shared_ptr<DataQueue<shared_ptr<AVPacket>>> packetQueue, shared_ptr<TexturePool> frameQueue){
    int err = 0;
    static constexpr auto frame_deleter = [](AVFrame* f){av_frame_free(&f);};
    // f.reset(av_frame_alloc());

    while(true) {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> frame;
        frameQueue->pop(frame);

        unique_ptr<AVFrame, decltype(frame_deleter)> f(av_frame_alloc(), frame_deleter);
        f->format = AV_PIX_FMT_NV12;
        f->width = codec_ctx->width;
        f->height = codec_ctx->height;
        if((err = av_hwframe_get_buffer(codec_ctx->hw_frames_ctx, f.get(), 0)) < 0){
            fprintf(stderr, "Error while transferring frame data to surface."
                        "Error code: %s.\n", av_err2str(err));
        }
        if(!f->hw_frames_ctx) {
            err = AVERROR(ENOMEM);
            fprintf(stderr, "Error code: %s.\n", av_err2str(err));
        }
        f->data[0] = (uint8_t*)frame.Get();

        // 5. 提交编码
        int ret = 0;
        if ((ret = avcodec_send_frame(codec_ctx.get(), f.get())) < 0) {
            av_frame_unref(f.get());
            fprintf(stderr, "Error code: %s\n", av_err2str(ret));
        }

        shared_ptr<AVPacket> pkt = shared_ptr<AVPacket>(av_packet_alloc(), [](AVPacket* pkt){
            av_packet_free(&pkt);
        });
        av_init_packet(pkt.get());
        // 6. 接收编码数据
        while (avcodec_receive_packet(codec_ctx.get(), pkt.get()) == 0) {
            // 处理编码后的数据
            packetQueue->push(pkt);
            // fprintf(stdout,"pkt size : %d\n", pkt->size);
            pkt.reset(av_packet_alloc(), [](AVPacket* pkt){
                av_packet_free(&pkt);
            });
        }
    }
    return;
}

shared_ptr<AVCodecContext> GPUEncoder::getCtx(){
    return codec_ctx;
}

Microsoft::WRL::ComPtr<ID3D11Device> GPUEncoder::getDevice(){
    return d3d_device;
}

Microsoft::WRL::ComPtr<ID3D11DeviceContext> GPUEncoder::getDeviceContext(){
    return d3d_context;
}