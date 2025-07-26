#ifndef ENCODER_HPP
#define ENCODER_HPP
#include "core.hpp"
#include "data_queue.hpp"
#include "texturepool.hpp"
class GPUEncoder{
    public:
        GPUEncoder();
        GPUEncoder(const GPUEncoder&) = delete;
        GPUEncoder(GPUEncoder&&) = delete;
        GPUEncoder& operator=(const GPUEncoder&) = delete;
        GPUEncoder& operator=(GPUEncoder&&) = delete;
        ~GPUEncoder() = default;

        void initEncode(int dst_width, int dst_height, int bitrate, int fps);
        // void encode(PacketQueue& packetQueue, TexturePool& frameQueue);
        void encode(shared_ptr<DataQueue<shared_ptr<AVPacket>>> packetQueue, shared_ptr<TexturePool> frameQueue);
        shared_ptr<AVCodecContext> getCtx();
        Microsoft::WRL::ComPtr<ID3D11Device> getDevice();
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> getDeviceContext();

    private:
        static constexpr auto hwDeviceCtx_deleter = [](AVBufferRef* buf){if(buf) av_buffer_unref(&buf);};
        shared_ptr<AVCodecContext> codec_ctx = nullptr;
        const AVCodec* codec = nullptr;
        unique_ptr<AVBufferRef, decltype(hwDeviceCtx_deleter)> hw_device_ctx; // 硬件设备上下文
        // d3d设备信息及上下文
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_context;
};
#endif