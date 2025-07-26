#include "texturepool.hpp"

TexturePool::TexturePool(size_t max_sz) : max_size(max_sz) {}

TexturePool::~TexturePool(){
    if(!frames.isClose()) frames.close();
}

void TexturePool::push(Microsoft::WRL::ComPtr<ID3D11Texture2D> const& frame){
    frames.push(frame);
}

void TexturePool::pop(Microsoft::WRL::ComPtr<ID3D11Texture2D> &frame){
    frames.pop(frame);
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> TexturePool::CreateSharedTexture(Microsoft::WRL::ComPtr<ID3D11Device> device,
                                                                     Microsoft::WRL::ComPtr<ID3D11Texture2D> image)
{
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
    
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture = nullptr;
    device->CreateTexture2D(&desc, nullptr, &texture);
    return texture;
}