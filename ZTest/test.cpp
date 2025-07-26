#include "encoder.hpp"
#include "control.hpp"
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wincodec.h>
#include <comdef.h>
using namespace std;

// 全局资源
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11InputLayout* g_pInputLayout = nullptr;
ID3D11SamplerState* g_pSamplerState = nullptr;
ID3D11BlendState* g_pBlendState = nullptr;
ID3D11Buffer* g_pConstantBuffer = nullptr;
ID3D11Texture2D* g_pMouseTexture = nullptr;
ID3D11ShaderResourceView* g_pMouseSRV = nullptr;
int width = 0, height = 0; // 设置原分辨率

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

HRESULT GPUCapture::initMouseRendering(){
    HRESULT hr;

    // 创建顶点着色器
    ID3DBlob* vsBlob = nullptr;
    const char* vsCode = R"(      
        struct VS_IN {
            float3 pos : POSITION;
            float2 tex : TEXCOORD0;
        };
        
        struct PS_IN {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
        };
        
        PS_IN main(VS_IN input) {
            PS_IN output;
            output.pos = float4(input.pos, 1.0f);
            output.tex = input.tex;
            return output;
        }
    )";
    
    hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "main", 
                    "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &vsBlob, nullptr);
    if (FAILED(hr)) return hr;
    
    hr = d3d_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), 
                                    nullptr, &g_pVertexShader);
    if (FAILED(hr)) return hr;
    
    // 创建像素着色器
    ID3DBlob* psBlob = nullptr;
    const char* psCode = R"(
        Texture2D mouseTex : register(t0);
        SamplerState samLinear : register(s0);
        
        struct PS_IN {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
        };
        
        float4 main(PS_IN input) : SV_TARGET {
            return mouseTex.Sample(samLinear, input.tex);
        }
    )";
    
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", 
                    "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &psBlob, nullptr);
    if (FAILED(hr)) return hr;
    
    hr = d3d_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), 
                                   nullptr, &g_pPixelShader);
    if (FAILED(hr)) return hr;
    
    // 创建输入布局
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    
    hr = d3d_device->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(), 
                                  vsBlob->GetBufferSize(), &g_pInputLayout);
    vsBlob->Release();
    psBlob->Release();

    if(FAILED(hr)) return hr;

    // 创建采样器状态
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    hr = d3d_device->CreateSamplerState(&sampDesc, &g_pSamplerState);
    if (FAILED(hr)) return hr;
    
    // 创建混合状态
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    
    hr = d3d_device->CreateBlendState(&blendDesc, &g_pBlendState);

    return hr;
}


// 初始化GPU绘制资源
HRESULT InitializeGPURendering(int screenWidth, int screenHeight) {
    HRESULT hr = S_OK;
    
    // 创建鼠标纹理
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = point_info.Width; // 暂时固定
    texDesc.Height = point_info.Height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_IMMUTABLE;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    
    D3D11_SUBRESOURCE_DATA pInitialData{};
    pInitialData.pSysMem = t.data();
    pInitialData.SysMemPitch = point_info.Pitch;
    hr = d3d_device->CreateTexture2D(&texDesc, &pInitialData, &g_pMouseTexture);
    if (FAILED(hr)) return hr;
    
    // 创建鼠标纹理的SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    
    hr = d3d_device->CreateShaderResourceView(g_pMouseTexture, &srvDesc, &g_pMouseSRV);
    
    return hr;
}

HRESULT DrawMouseOnCapturedTexture(
    ID3D11Texture2D* pScreenTexture,
    int screenWidth, int screenHeight)
{
    HRESULT hr = S_OK;
    
    // 1. 创建屏幕纹理的渲染目标视图
    ID3D11RenderTargetView* pRTV = nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    
    hr = d3d_device->CreateRenderTargetView(pScreenTexture, &rtvDesc, &pRTV);
    if (FAILED(hr)) return hr;
    
    // 2. 设置渲染目标
    d3d_context->OMSetRenderTargets(1, &pRTV, nullptr);
    
    // 3. 设置视口
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<FLOAT>(g_mouseSize.cx);
    viewport.Height = static_cast<FLOAT>(g_mouseSize.cy);
    viewport.TopLeftX = g_currentMousePosition.Position.x;
    viewport.TopLeftY = g_currentMousePosition.Position.y;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    d3d_context->RSSetViewports(1, &viewport);
    
    // 4. 设置混合状态
    float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    UINT sampleMask = 0xffffffff;
    d3d_context->OMSetBlendState(g_pBlendState, blendFactor, sampleMask);
    
    // 5. 设置着色器
    d3d_context->VSSetShader(g_pVertexShader, nullptr, 0);
    d3d_context->PSSetShader(g_pPixelShader, nullptr, 0);
    
    // 6. 设置输入布局
    d3d_context->IASetInputLayout(g_pInputLayout);
    
    // 7. 设置鼠标纹理和采样器
    d3d_context->PSSetShaderResources(0, 1, &g_pMouseSRV);
    d3d_context->PSSetSamplers(0, 1, &g_pSamplerState);
    
    // 8. 设置图元拓扑
    d3d_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    
    // 9. 准备顶点数据
    const float screenWidthF = static_cast<float>(screenWidth);
    const float screenHeightF = static_cast<float>(screenHeight);
    const float mouseWidthF = static_cast<float>(g_mouseSize.cx);
    const float mouseHeightF = static_cast<float>(g_mouseSize.cy);
    
    // 计算鼠标位置的归一化坐标 (0-1)
    float posX = static_cast<float>(g_currentMousePosition.Position.x) / screenWidthF;
    float posY = static_cast<float>(g_currentMousePosition.Position.y) / screenHeightF;
    float sizeX = mouseWidthF / screenWidthF;
    float sizeY = mouseHeightF / screenHeightF;
    
    SimpleVertex vertices[4] = {
        { DirectX::XMFLOAT3(-1,        1,         0.5f), DirectX::XMFLOAT2(0.0f, 0.0f) },
        { DirectX::XMFLOAT3(1, 1,         0.5f), DirectX::XMFLOAT2(1.0f, 0.0f) },
        { DirectX::XMFLOAT3(-1,        -1, 0.5f), DirectX::XMFLOAT2(0.0f, 1.0f) },
        { DirectX::XMFLOAT3(1, -1, 0.5f), DirectX::XMFLOAT2(1.0f, 1.0f) }
    };
    
    // 11. 创建动态顶点缓冲区并绘制
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    ID3D11Buffer* pVertexBuffer = nullptr;
    hr = d3d_device->CreateBuffer(&bufferDesc, nullptr, &pVertexBuffer);
    if (SUCCEEDED(hr)) {
        D3D11_MAPPED_SUBRESOURCE vertexMapped;
        hr = d3d_context->Map(pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vertexMapped);
        if (SUCCEEDED(hr)) {
            memcpy(vertexMapped.pData, vertices, sizeof(vertices));
            d3d_context->Unmap(pVertexBuffer, 0);
            
            // 设置顶点缓冲区
            UINT stride = sizeof(SimpleVertex);
            UINT offset = 0;
            d3d_context->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
            
            // 绘制鼠标
            d3d_context->Draw(4, 0);
        }
        
        pVertexBuffer->Release();
    }
    
    // 12. 清理资源
    ID3D11ShaderResourceView* nullSRVs[1] = { nullptr };
    d3d_context->PSSetShaderResources(0, 1, nullSRVs);
    d3d_context->OMSetRenderTargets(0, nullptr, nullptr);
    
    if(pRTV) pRTV->Release();
    if(pVertexBuffer) pVertexBuffer->Release();
    
    return hr;
}

void convert(TexturePool* frames, ID3D11Texture2D* src) {
    ID3D11Texture2D *dst;
    // 为输入纹理数据创建srv
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1; // 只使用第一级mip贴图

    // 创建NV12纹理的无序访问视图UAV_DESC(只有这种视图支持计算着色器进行读写)
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    D3D11_TEXTURE2D_DESC srcTextDesc;
    src->GetDesc(&srcTextDesc);

    if(update_flag) {
        InitializeGPURendering(srcTextDesc.Width, srcTextDesc.Height);

        // 绘制鼠标纹理到屏幕纹理
        DrawMouseOnCapturedTexture(src, srcTextDesc.Width, srcTextDesc.Height);
    }

    // 记录原分辨率
    width = srcTextDesc.Width;
    height = srcTextDesc.Height;

    srcTextDesc.Format = DXGI_FORMAT_NV12;
    srcTextDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // 为了绑定UAV需要这个标志
    srcTextDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    srcTextDesc.Width = t_width; // 设置帧分辨率为缩放后的分辨率
    srcTextDesc.Height = t_height;
    srcTextDesc.MipLevels = 1;
    srcTextDesc.SampleDesc = {1, 0};
    srcTextDesc.Usage = D3D11_USAGE_DEFAULT;
    srcTextDesc.CPUAccessFlags = 0;
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
    d3d_context->CSSetShader(t_shader, nullptr, 0);
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
    frames->ava_push(dst);

    // 释放资源
    src->Release();
    rgbaSRV->Release();
    yPlaneUAV->Release();
    uvPlaneUAV->Release();
    pQuery->Release();
    g_pMouseTexture->Release();
    // if(g_pMouseSRV) g_pMouseSRV->Release();
}
