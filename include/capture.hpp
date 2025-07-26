#ifndef CAPTURE_HPP
#define CAPTURE_HPP

#include "core.hpp"
#include "texturepool.hpp"

class GPUCapture{
    public:
        GPUCapture(int _fps = 60);
        GPUCapture(const GPUCapture&) = delete;
        GPUCapture(GPUCapture&&) = delete;
        GPUCapture& operator=(const GPUCapture&) = delete;
        GPUCapture& operator=(GPUCapture&&) = delete;
        ~GPUCapture() = default;

        bool init(Microsoft::WRL::ComPtr<ID3D11Device> d3d_device_, Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_context_);
        void CaptureScreen(shared_ptr<TexturePool> frames, int dst_width, int dst_height);
        void initComputeShader(const char* file_path);
        int getSrcWidth();
        int getSrcHeight();

        // 常量缓冲区结构
        struct ConstantBuffer {
            DirectX::XMFLOAT4X4 worldViewProj;
            DirectX::XMFLOAT4 mousePosition;
        };

        // 顶点结构
        struct SimpleVertex {
            DirectX::XMFLOAT3 Pos;
            DirectX::XMFLOAT2 TexCoord;
        };

    private:
        HRESULT initMouseRendering();
        HRESULT InitializeGPURendering();
        HRESULT DrawMouseOnCapturedTexture(Microsoft::WRL::ComPtr<ID3D11Texture2D> pScreenTexture);
        void convert(shared_ptr<TexturePool> frames, Microsoft::WRL::ComPtr<ID3D11Texture2D> src,
                                        int dst_width, int dst_height);

        int fps = 0;
        // int64_t start_time = 0;
        Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication; // dxgi图像纹理接口
        Microsoft::WRL::ComPtr<IDXGIResource> dxgi_res; // dxgi图像纹理资源
        // unique_ptr<ID3D11Texture2D*> internal_frame; // 内部缓冲帧
        Microsoft::WRL::ComPtr<ID3D11VertexShader> g_pVertexShader; // 鼠标顶点着色器
        Microsoft::WRL::ComPtr<ID3D11PixelShader> g_pPixelShader; // 鼠标像素着色器
        Microsoft::WRL::ComPtr<ID3D11InputLayout> g_pInputLayout; // 混合着色器输入布局
        Microsoft::WRL::ComPtr<ID3D11SamplerState> g_pSamplerState; // 混合采样器
        Microsoft::WRL::ComPtr<ID3D11BlendState> g_pBlendState; // 设置混合状态
        Microsoft::WRL::ComPtr<ID3D11Texture2D> g_pMouseTexture; // 鼠标纹理数据
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> g_pMouseSRV; // 鼠标纹理src
        Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader; // 缩放采样器
        DXGI_OUTDUPL_POINTER_POSITION g_currentMousePosition; // 鼠标当前位置
        SIZE g_mouseSize; // 鼠标尺寸大小
        DXGI_OUTDUPL_POINTER_SHAPE_INFO point_info; // 原始鼠标上下文信息
        vector<uint8_t> g_mousedata; // 原始鼠标数据信息
        bool update_flag;  // 判断当前屏幕鼠标纹理是否可见
        int screenWidth = 0, screenHeight = 0; // 设置原分辨率

        // d3d设备信息及上下文
        Microsoft::WRL::ComPtr<ID3D11Device> d3d_device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d_context;
};
#endif