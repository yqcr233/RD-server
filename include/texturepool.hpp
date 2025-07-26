#ifndef TEXTURE_HPP
#define TEXTURE_HPP
#include "core.hpp"
#include "data_queue.hpp"

class TexturePool{
    public:
        TexturePool(size_t max_sz = 1);
        TexturePool(const TexturePool&) = delete;
        TexturePool(TexturePool&&) = delete;
        TexturePool& operator=(const TexturePool&) = delete;
        TexturePool& operator=(TexturePool&&) = delete;
        ~TexturePool();

        // 控制屏幕纹理共享队列
        void push(Microsoft::WRL::ComPtr<ID3D11Texture2D> const& frame);
        void pop(Microsoft::WRL::ComPtr<ID3D11Texture2D> &frame);
        Microsoft::WRL::ComPtr<ID3D11Texture2D> CreateSharedTexture(Microsoft::WRL::ComPtr<ID3D11Device> device,
                                                                     Microsoft::WRL::ComPtr<ID3D11Texture2D> image);
    private:
        // DataQueue<ID3D11Texture2D*> frames{max_size};
        DataQueue<Microsoft::WRL::ComPtr<ID3D11Texture2D>> frames;
        size_t max_size;
};
#endif