#include "encoder.hpp"
#include "control.hpp"
#include <vector>

using namespace std;
ID3D11Device* d3d_device = nullptr;
ID3D11DeviceContext* d3d_context = nullptr;

int t_width = 0, t_height = 0; // 设置缩放后分辨率
ID3D11ComputeShader* t_shader = nullptr;
DXGI_OUTDUPL_POINTER_POSITION g_currentMousePosition;
SIZE g_mouseSize = {0, 0};
DXGI_OUTDUPL_POINTER_SHAPE_INFO point_info;
vector<uint8_t> t;
bool update_flag = 0;

void PacketQueue::push(AVPacket* packet){
    {
        unique_lock<std::mutex> lock(mutex);
        not_full.wait(lock, [&](){
            return packets.size()<max_size;
        });

        // AVPacket* p = av_packet_clone(packet);
        AVPacket* p = av_packet_alloc();
        av_packet_move_ref(p, packet);
        if(!p) return;
        packets.push(p);
        // fprintf(stdout, ("push packet " + to_string(packets.size()) + "\n").c_str());
        // fprintf(stdout, "/n");
    }
    not_empty.notify_all();
    return;
}   

bool PacketQueue::pop(AVPacket* packet){
    {
        unique_lock<std::mutex> lock(mutex);
        not_empty.wait(lock, [&](){
            return !packets.empty();
        });

        av_packet_move_ref(packet, packets.front());
        av_packet_free(&packets.front());
        packets.pop();

        // fprintf(stdout, ("pop packet " + to_string(packets.size()) + "\n").c_str());
        // fprintf(stdout, "/n");
    }
    not_full.notify_all();
    return true;
}

bool GPUCapture::init(){
    while(!d3d_device || !d3d_context) ;

    HRESULT hr = S_OK;
    // 根据d3d设备获取对应dxgi设备接口
    IDXGIDevice* dxgi_device = nullptr;
    HRESULT hrr = d3d_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device));

    // 获取dxgi设备适配器
    IDXGIAdapter* dxgi_adapter = nullptr;
    hrr = dxgi_device->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgi_adapter));
    dxgi_device->Release();

    // 获取设备输出接口
    IDXGIOutput* dxgi_output = nullptr;
    UINT _output_idx = 0; // 输出索引设置
    hrr = dxgi_adapter->EnumOutputs(_output_idx, &dxgi_output);
    dxgi_adapter->Release();

    // 获取输出设备描述
    DXGI_OUTPUT_DESC _output_des;
    dxgi_output->GetDesc(&_output_des);

    // 创建duplication接口
    IDXGIOutput1* dxgi_output1 = nullptr;
    hr = dxgi_output->QueryInterface(__uuidof(dxgi_output1), reinterpret_cast<void**>(&dxgi_output1));
    hr = dxgi_output1->DuplicateOutput(d3d_device, &duplication);
    dxgi_output->Release();
    dxgi_output1->Release();
    return true;
}

void GPUCapture::CaptureScreen(TexturePool& frames, AVCodecContext* codec_ctx){
    ID3D11SamplerState* bilinearSampler;
    D3D11_SAMPLER_DESC sample_desc;
    sample_desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sample_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sample_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    d3d_device->CreateSamplerState(&sample_desc, &bilinearSampler);
    d3d_context->PSSetSamplers(0, 1, &bilinearSampler);

    while(true) {
        DXGI_OUTDUPL_FRAME_INFO frame_info;

        duplication->ReleaseFrame(); // 获取新帧之前释放帧， 防止在处理上一帧时duplication接口中的帧数据出现变化
        HRESULT hr = duplication->AcquireNextFrame(16, &frame_info, &dxgi_res); // 获取一帧图像资源
        
        if(frame_info.PointerShapeBufferSize) { // 鼠标状态更新则重画，否则用之前的数据
            t.resize(frame_info.PointerShapeBufferSize);
            UINT a;
            // 获取屏幕鼠标信息
            duplication->GetFramePointerShape(frame_info.PointerShapeBufferSize, t.data(), &a, &point_info);
            if(point_info.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR) {
                for(int i = 3; i < t.size(); i += 4) { // 将鼠标框背景的a值设为0(完全透明)
                    if(t[i-1] < 127) t[i] = 0;
                }
            }
        }

        // 判断是否移动和是否绘制鼠标
        if(frame_info.LastMouseUpdateTime.QuadPart > 0) {
            update_flag = frame_info.PointerPosition.Visible;
            g_currentMousePosition = frame_info.PointerPosition;
        }

        if(point_info.Height && point_info.Width){
            g_mouseSize.cx = point_info.Width;
            g_mouseSize.cy = point_info.Height;
            // fprintf(stdout, "type %d width %d height %d\n", point_info.Type, point_info.Width, point_info.Height);
        }
        if(FAILED(hr)) { // 没有获取返回上一帧
            // ID3D11Texture2D* _image = nullptr;
            // if(internal_frame) {
            //     d3d_context->CopyResource(_image, internal_frame);
            //     convert(&frames, _image);
            //     _image->Release();
            // }
            continue;
        }

        ID3D11Texture2D* image, *_image;
        // 获取一帧图像纹理
        hr = dxgi_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&_image));
        image = frames.CreateSharedTexture(d3d_device, _image);
        // 将从纹理资源接口中屏幕帧数据拷贝到gpu缓冲区中
        d3d_context->CopyResource(image, _image);

        // if(!internal_frame) { // 新帧填充帧缓冲
        //     internal_frame->Release();
        //     d3d_context->CopyResource(internal_frame, _image);
        // }
        // if(internal_frame) internal_frame->Release();
        // d3d_context->CopyResource(internal_frame, _image);
        convert(&frames, image);
        
        // frames.push(image);
        _image->Release();

        // this_thread::sleep_for(chrono::milliseconds(1000/fps));
    }
    return;
}

ID3D11Texture2D* TexturePool::CreateSharedTexture(ID3D11Device* device, ID3D11Texture2D* image){
    D3D11_TEXTURE2D_DESC desc;
    image->GetDesc(&desc);
    desc.MipLevels = 1;
    desc.BindFlags = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc = {1, 0};
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    
    ID3D11Texture2D* texture = nullptr;
    device->CreateTexture2D(&desc, nullptr, &texture);
    return texture;
}

void TexturePool::push(ID3D11Texture2D* frame){
    {
        unique_lock<std::mutex> lock(mutex);
        not_full.wait(lock, [this]{
            return frames.size() < max_size;
        });

        frames.push(frame);
        // fprintf(stdout, ("push pre frame " + to_string(frames.size()) + "\n").c_str());
    }
    not_empty.notify_all();
    return;
}

ID3D11Texture2D* TexturePool::pop(){
    ID3D11Texture2D* frame;
    {
        unique_lock<std::mutex> lock(mutex);
        not_empty.wait(lock, [this]{
            return !frames.empty();
        });

        frame = frames.front();
        frames.pop();
        // fprintf(stdout, ("pop pre frame " + to_string(frames.size()) + "\n").c_str());
    }
    not_full.notify_all();
    return frame;
}

void TexturePool::ava_push(ID3D11Texture2D* frame) {
    {
        unique_lock<std::mutex> lock(ava_mutex);
        ava_not_full.wait(lock, [this] {
            return ava_frames.size() < max_size;
        });

        ava_frames.push(frame);
        // fprintf(stdout, ("push frame " + to_string(ava_frames.size()) + "\n").c_str());
    }
    ava_not_empty.notify_all();
}

ID3D11Texture2D* TexturePool::ava_pop() {
    ID3D11Texture2D* frame;
    {
        unique_lock<std::mutex> lock(ava_mutex);
        ava_not_empty.wait(lock, [this] {
            return !ava_frames.empty();
        });

        frame = ava_frames.front();
        // ava_frames.front()->Release();
        ava_frames.pop();
        // fprintf(stdout, ("pop frame " + to_string(ava_frames.size()) + "\n").c_str());
    }
    ava_not_full.notify_all();
    return frame;
}

void TexturePool::ConvertRGBAtoNV12(ID3D11ComputeShader* shader){
    ID3D11SamplerState* bilinearSampler;
    D3D11_SAMPLER_DESC sample_desc;
    sample_desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sample_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sample_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    d3d_device->CreateSamplerState(&sample_desc, &bilinearSampler);
    d3d_context->PSSetSamplers(0, 1, &bilinearSampler);

    // 为输入纹理数据创建srv
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1; // 只使用第一级mip贴图

    // 创建NV12纹理的无序访问视图UAV_DESC(只有这种视图支持计算着色器进行读写)
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    
    while(true) {
        ID3D11Texture2D* src, *dst;
        src = pop(); // 获取一帧屏幕纹理
        if(src) {
            D3D11_TEXTURE2D_DESC srcTextDesc;
            src->GetDesc(&srcTextDesc);
            srcTextDesc.Format = DXGI_FORMAT_NV12;
            srcTextDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // 为了绑定UAV需要这个标志
            srcTextDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
            srcTextDesc.Width = t_width; // 设置帧分辨率为缩放后的分辨率
            srcTextDesc.Height = t_height;
            d3d_device->CreateTexture2D(&srcTextDesc, nullptr, &dst); // 创建目的纹理

            ID3D11ShaderResourceView* rgbaSRV = nullptr;
            d3d_device->CreateShaderResourceView(src, &srvDesc, &rgbaSRV);

            // 创建Y平面UAV
            uavDesc.Format = DXGI_FORMAT_R8_UNORM; // Y平面
            ID3D11UnorderedAccessView* yPlaneUAV = nullptr;
            d3d_device->CreateUnorderedAccessView(dst, &uavDesc, &yPlaneUAV);

            // 创建UV平面UAV
            uavDesc.Format = DXGI_FORMAT_R8G8_UNORM; // UV平面
            ID3D11UnorderedAccessView* uvPlaneUAV = nullptr;
            d3d_device->CreateUnorderedAccessView(dst, &uavDesc, &uvPlaneUAV);

            // 绑定资源端到计算着色器
            d3d_context->CSSetShader(shader, nullptr, 0);
            d3d_context->CSSetShaderResources(0, 1, &rgbaSRV);

            // 绑定输出目标
            ID3D11UnorderedAccessView* uavs[2] = {yPlaneUAV, uvPlaneUAV};
            d3d_context->CSSetUnorderedAccessViews(0, 2,uavs, nullptr);

            // 分派着色器, 根据纹理长和宽计算分派的线程组数量
            UINT groupsX = (srcTextDesc.Width + 7) / 8; 
            UINT groupsY = (srcTextDesc.Height + 7) / 8;
            d3d_context->Dispatch(groupsX, groupsY, 1);
            
            // 计算完成之后解除绑定，即绑上空的UAV
            ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
            d3d_context->CSSetShaderResources(0, 1, nullSRVs);
            ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr, nullptr };
            d3d_context->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);

            // 创建同步查询对象等待之前所有命令全部执行完毕，直到End(pQuery)
            ID3D11Query* pQuery = nullptr;
            D3D11_QUERY_DESC queryDesc = { D3D11_QUERY_EVENT, 0 };
            d3d_device->CreateQuery(&queryDesc, &pQuery);
            d3d_context->End(pQuery);
            
            while(d3d_context->GetData(pQuery, nullptr, 0, 0) == S_FALSE){// 循环等待直到查询完成(也表示计算完成)
                // this_thread::sleep_for(chrono::milliseconds(1));
            }
            // 计算完成后将转换后的纹理加入ava_frames队列
            ava_push(dst);

            // 释放资源
            rgbaSRV->Release();
            yPlaneUAV->Release();
            uvPlaneUAV->Release();
            pQuery->Release();
            src->Release(); // 也将使用完毕的屏幕帧数据释放
        }
    }

    bilinearSampler->Release();

}


void get_err(int ret){
    char err[128];
    av_strerror(ret, err, sizeof(err));
    cout<<"++++++++++++++++++++++"<<endl;
    cout<<"error: "<<err<<endl;
    cout<<"++++++++++++++++++++++"<<endl;
    fprintf(stdout, "error: ");
    fprintf(stdout, "%s\n", err);
}

void GPUEncoder::encode(PacketQueue& packetQueue, TexturePool& frameQueue) {
    AVFrame* f = av_frame_alloc();
    // f->format = AV_PIX_FMT_D3D11;

    int err = 0;

    IDXGIResource* dxgiResource = nullptr;
    AVPacket* pkt = av_packet_alloc();
    av_init_packet(pkt);

    while(true) {
        ID3D11Texture2D* frame = frameQueue.ava_pop();
        f->format = AV_PIX_FMT_NV12;
        f->width = codec_ctx->width;
        f->height = codec_ctx->height;

        if((err = av_hwframe_get_buffer(codec_ctx->hw_frames_ctx, f, 0)) < 0){
            fprintf(stderr, "Error while transferring frame data to surface."
                        "Error code: %s.\n", av_err2str(err));
        }
        if(!f->hw_frames_ctx) {
            err = AVERROR(ENOMEM);
            fprintf(stderr, "Error code: %s.\n", av_err2str(err));
        }

        // // 获取共享句柄
        // frame->QueryInterface(__uuidof(IDXGIResource), (void**)&dxgiResource);
        // HANDLE shareHandle = nullptr;
        // dxgiResource->GetSharedHandle(&shareHandle);
        // // dxgiResource->AddRef();
        // // 设置帧的纹理引用，AV_PIX_FMT_D3D11表示该帧数据是存放在d3d11纹理中的
        // // 所以这里和软编码不同，不存实际分量数据而是指向数据和纹理参数的句柄和指针
        // f->data[0] = (uint8_t*)shareHandle; // 允许跨进程或跨API共享纹理资源
        // f->data[1] = (uint8_t*)dxgiResource; // 这是一个COM对象，用于访问纹理资源的方法和属性
        // AVD3D11FrameDescriptor* desc = 
        //     reinterpret_cast<AVD3D11FrameDescriptor*>(f->data[0]);
        // 直接获取纹理资源
        // ID3D11Texture2D* dstTex = (ID3D11Texture2D*)f->data[0];
        // d3d_context->CopyResource(dstTex, frame);
        // frame->Release();
        f->data[0] = (uint8_t*)frame;

        // 5. 提交编码
        int ret = 0;
        if ((ret = avcodec_send_frame(codec_ctx, f)) < 0) {
            av_frame_unref(f);
            fprintf(stderr, "Error code: %s\n", av_err2str(ret));
        }

        // 释放编码帧资源
        av_frame_unref(f);
        frame->Release();
        // dxgiResource->Release();
        
        // 6. 接收编码数据
        while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
            // 处理编码后的数据
            packetQueue.push(pkt);
            // fwrite(pkt.data, 1, pkt.size, output_file);
            av_packet_unref(pkt);
        }
    }

    if(f) av_frame_free(&f);
    if(pkt) av_packet_free(&pkt);
    dxgiResource->Release();
    return;
}

void GPUEncoder::initEncode(int width, int height, int bitrate, int fps) {
    t_width = width;
    t_height = height;

    codec = avcodec_find_encoder_by_name("hevc_amf");
    if(!codec) {
        fprintf(stdout, "not found codec\n");
        return;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if(!codec_ctx){
        fprintf(stdout, "fail to allocate codec context\n");
        return;
    }

    // 根据屏幕数据据配置编码参数并打开编码器
    codec_ctx->width = width; 
    codec_ctx->height = height;

    codec_ctx->time_base = {1, 90000}; 
    codec_ctx->framerate = {60, 1}; // 不设帧率宏块过多会花屏
    codec_ctx->bit_rate = bitrate;
    codec_ctx->gop_size = 3600;
    codec_ctx->max_b_frames = 0;
    codec_ctx->pix_fmt = AV_PIX_FMT_D3D11; // 设置输入帧数据为d3d11纹理输入

    AVDictionary* options = nullptr;
    
    int mode = 1;
    // 1. 替换 "veryfast" 为 AMF 的质量预设
    av_dict_set(&options, "quality", "speed", 0);  // 可选: speed, balanced, quality
    // av_dict_set_int(&options, "quality", 1, 0);  // 可选: speed, balanced, quality
    // 2. 替换 "high444" 为标准的 profile
    // av_dict_set(&options, "profile", "high", 0);   // 可选: baseline, main, high
    // av_dict_set_int(&options, "profile", 77, 0);
    // 设置低延迟编码
    // av_opt_set(codec_ctx->priv_data, "tune", "zerolatency", 0); 
    // av_dict_set_int(&options, "usage", 1, 0);
    av_dict_set(&options, "usage", "ultralowlatency", 0);

    // av_dict_set(&options, "preanalysis", "disabled", 0);       // 禁用预分析

    // // 码率控制优化
    // av_dict_set(&options, "rate_control", "cbr_latency", 0); // 低延迟CBR模式
    // av_dict_set(&options, "filler_data", "1", 0);          // 填充数据维持CBR
    // av_dict_set(&options, "enforce_hrd", "1", 0);          // 严格HRD合规

    // // GOP结构优化
    // av_dict_set(&options, "gops_per_idr", "0", 0);         // 每个I帧都是IDR
    // av_dict_set(&options, "header_insertion_mode", "idr", 0); // 每个IDR插入头
    // av_dict_set(&options, "max_au_size", "0", 0);          // 不限制访问单元大小

    // 设置硬件设备上下文
    int err = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_D3D11VA, NULL, NULL, 0);
    if (err < 0) {
        fprintf(stderr, "Failed to create a device. Error code: %s\n", av_err2str(err));
    }

    // 获取硬件设备及其上下文
    AVHWDeviceContext* hw_ctx = (AVHWDeviceContext*)hw_device_ctx->data;
    AVD3D11VADeviceContext* hw_d3d_ctx = (AVD3D11VADeviceContext*)hw_ctx->hwctx;
    d3d_device = hw_d3d_ctx->device;
    d3d_context = hw_d3d_ctx->device_context;
    d3d_device->AddRef();
    d3d_context->AddRef();

    // 设置编码器上下文帧缓冲区
    AVBufferRef* hw_frames_ref = nullptr;
    AVHWFramesContext* frames_ctx = nullptr;
    if(!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        fprintf(stderr, "Failed to create VAAPI frame context.\n");
    }
    frames_ctx = (AVHWFramesContext*)hw_frames_ref->data;
    auto d3d11Ctx = (AVD3D11VAFramesContext*)frames_ctx->hwctx;
    d3d11Ctx->BindFlags |= D3D11_BIND_RENDER_TARGET;
    frames_ctx->format = AV_PIX_FMT_D3D11;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width = width;
    frames_ctx->height = height;
    frames_ctx->initial_pool_size = 10;
    if((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        fprintf(stderr, "Failed to initialize frame context."
                "Error code: %s\n",av_err2str(err));
        av_buffer_unref(&hw_frames_ref);
    }
    codec_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if(!codec_ctx->hw_frames_ctx) {
        fprintf(stderr, "fail to create ctx->hw_frames_ctx");
    }
    av_buffer_unref(&hw_frames_ref);
    
    // codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    if(avcodec_open2(codec_ctx, codec, &options)<0){
        fprintf(stdout, "fail ot open codec\n"); 
    } 
    av_dict_free(&options); 
}

void TexturePool::initTexturePool() {
    // 初始化时加载计算着色器，并开启着色线程
    // 运行时加载cso文件
    ifstream file("D:\\program\\C++\\PlayerServer\\GPU\\dx11.cso", std::ios::binary | std::ios::ate);
    if (!file)
    {
        // 如果打开失败，抛出一个异常或者使用错误代码处理
        throw std::runtime_error("failed to open file");
    }

    // 获取文件大小
    streamsize sz = file.tellg();
    file.seekg(ios::beg);

    // 分配内存并读取文件内容
    vector<uint8_t> bytecode(sz);
    file.read(reinterpret_cast<char*>(bytecode.data()), sz);
    // 运行时加载计算着色器
    d3d_device->CreateComputeShader(
        bytecode.data(),
        bytecode.size(),
        nullptr,    
        &shader
    );
    t_shader = shader;
    
    // 后台开始转换线程
    // convert_func = make_unique<thread>(
    //     bind(&TexturePool::ConvertRGBAtoNV12, 
    //         this, 
    //         placeholders::_1),
    //         shader
    // );
}

