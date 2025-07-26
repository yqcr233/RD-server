#include <iostream>
#include <string>
#include <ctime>
#include <thread>
#include <queue>
#include <deque>
#include <condition_variable>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <winsock2.h> // windows socket api定义
#include <ws2tcpip.h> // windows socket api定义
#include <windows.h> // windows api定义，如句柄、线程函数
#include <fstream>
#include <functional>
extern "C"{
    #include <libavcodec/avcodec.h>
    #include<libavformat/avformat.h>
    #include <libavdevice/avdevice.h>
    #include <libavfilter/avfilter.h>
    #include <libavutil/avutil.h>
    #include<libswscale/swscale.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>
    #include <libavutil/hwcontext_d3d11va.h>
    #include <libswresample/swresample.h>
    #include <libavutil/time.h>
    #include <d3d11.h>
    #include <dxgi.h>
    #include <dxgi1_2.h>
}
using namespace std;

class TexturePool;
class PacketQueue;

extern ID3D11Device* d3d_device;
extern ID3D11DeviceContext* d3d_context;

extern int t_width, t_height;
extern ID3D11ComputeShader* t_shader;

// 鼠标纹理全局资源
extern ID3D11VertexShader* g_pVertexShader; // 顶点着色器
extern ID3D11PixelShader* g_pPixelShader ; // 像素着色器
extern ID3D11InputLayout* g_pInputLayout ; //
extern ID3D11SamplerState* g_pSamplerState ;
extern ID3D11BlendState* g_pBlendState ;
extern ID3D11Buffer* g_pConstantBuffer ;
extern ID3D11Texture2D* g_pMouseTexture ;
extern ID3D11ShaderResourceView* g_pMouseSRV ;
extern DXGI_OUTDUPL_POINTER_POSITION g_currentMousePosition;
extern SIZE g_mouseSize;
extern DXGI_OUTDUPL_POINTER_SHAPE_INFO point_info;
extern vector<uint8_t> t;
extern bool update_flag;  


/**
 * 捕获屏幕数据
 */
class GPUCapture{
    public:
        GPUCapture(int _fps = 60):fps(_fps){
            // 初始化GPU
            init();
            initMouseRendering();
        }
        ~GPUCapture() {
            if(duplication) duplication->Release();
            if(dxgi_res) dxgi_res->Release();
        }
        bool init();
        void CaptureScreen(TexturePool& frames, AVCodecContext* codec_ctx);
        HRESULT initMouseRendering();

    private:
        int fps = 0;
        int64_t start_time = 0;
        IDXGIOutputDuplication* duplication = nullptr;
        IDXGIResource* dxgi_res = nullptr;
        ID3D11Texture2D* internal_frame = nullptr;
};

/**
 * 共享纹理池
 */
class TexturePool{
    public:
        TexturePool(int max_sz = 1):max_size(max_sz){
            initTexturePool();
        }
        ~TexturePool(){
            while(!frames.empty()) {
                ID3D11Texture2D* f = frames.front();
                f->Release();
                frames.pop();
            }
            while(!ava_frames.empty()) {
                ID3D11Texture2D* f = ava_frames.front();
                f->Release();
                ava_frames.pop();
            }
            if(shader) shader->Release();
            // if(convert_func) convert_func.release();
        }

        void initTexturePool();

        // 控制屏幕纹理共享队列
        void push(ID3D11Texture2D* frame);
        ID3D11Texture2D* pop();
        // 控制转换后纹理队列
        void ava_push(ID3D11Texture2D* frame);
        ID3D11Texture2D* ava_pop();
        ID3D11Texture2D* CreateSharedTexture(ID3D11Device* device, ID3D11Texture2D* image);
        void ConvertRGBAtoNV12(ID3D11ComputeShader* shader);

    private:
        queue<ID3D11Texture2D*> frames; // 共享纹理队列
        std::mutex mutex;
        condition_variable not_full;
        condition_variable not_empty;
        queue<ID3D11Texture2D*> ava_frames; // 类型转换后的共享纹理队列
        std::mutex ava_mutex;
        condition_variable ava_not_full;
        condition_variable ava_not_empty;
        size_t max_size;
        unique_ptr<thread> convert_func;
        ID3D11ComputeShader* shader;
        ID3D11Texture2D* internal_frame;
};

/**
 * 硬编码器
 */
class GPUEncoder{
    public:
        GPUEncoder(int width, int height, int bitrate, int fps){
            initEncode(width, height, bitrate, fps);
        }
        ~GPUEncoder(){
            if(codec_ctx) {
                avcodec_close(codec_ctx);
                avcodec_free_context(&codec_ctx);
                if(d3d_device) d3d_device->Release();
                if(d3d_context) d3d_context->Release();
            }
        }
        void initEncode(int width, int height, int bitrate, int fps);
        void encode(PacketQueue& packetQueue, TexturePool& frameQueue);
        AVCodecContext* getCtx(){
            return codec_ctx;
        }

    private:
        AVCodecContext* codec_ctx = nullptr;
        const AVCodec* codec = nullptr;
        AVBufferRef* hw_device_ctx; // 硬件设备上下文
};

/**
 * packet队列
 */
class PacketQueue{
    public:
        explicit PacketQueue(int _max_size = 1):max_size(_max_size){}
        ~PacketQueue(){
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

void convert(TexturePool* frames, ID3D11Texture2D* src);


