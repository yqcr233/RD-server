// 着色器常量，表示每个线程组处理像素块的大小
#define BLOCK_SIZE 8

Texture2D<float4> InputTexture: register(t0); // 输入纹理绑定到t0寄存器
SamplerState      bilinearSampler: register(s0); // 添加双线性采样器

RWTexture2D<float> OutputY: register(u0); // Y平面输出，绑定到u0寄存器
RWTexture2D<float2> OutputUV: register(u1); // UV平面输出，绑定到u1寄存器

// 使用计算机常用的BT.601标准
static const float3 YCoeff = float3(0.299, 0.587, 0.114);
static const float3 UCoeff = float3(-0.1687, -0.3313, 0.5);
static const float3 VCoeff = float3(0.5, -0.4187, -0.0813);

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]  // 设置线程组大小，每个线程处理一个像素
void CSMain( uint3 DTid : SV_DispatchThreadID ){
    // 获取目标分辨率尺寸（缩放后）
    uint dstWidth, dstHeight;
    OutputY.GetDimensions(dstWidth, dstHeight);

    uint width, height;
    InputTexture.GetDimensions(width, height);

    // 检查当前线程是否超出纹理边界，避免处理超出纹理边界的像素
    if (DTid.x >= dstWidth || DTid.y >= dstHeight) return;
    // if(DTid.x >= width || DTid.y >= height) return;

    // float2 scale = {
    //     dstWidth / width,
    //     dstHeight / height
    // };

    // float2 inverseScale = {
    //     1.0 / (scale.x * width),
    //     1.0 / (scale.y * height)
    // };

    // 计算原始纹理中的采样位置（应用缩放）
    float2 srcUV = float2(
        (DTid.x + 0.5) / dstWidth,
        (DTid.y + 0.5) / dstHeight
    );
    // 采样源纹理（双线性插值）
    float4 rgba = InputTexture.SampleLevel(bilinearSampler, srcUV, 0);

    // 获取线程输入纹理像素rgba值，并当前位置像素Y分量值，并送入Y平面相应位置
    // float4 rgba = InputTexture[DTid.xy];

    float y = dot(rgba, YCoeff);
    y = saturate(y);
    OutputY[DTid.xy] = y * 219.0/255.0 + 16.0/255.0;  // 范围映射到[16/255, 235/255]

    if(DTid.x % 2 == 0 && DTid.y % 2 == 0){ // 当线程位于像素块左上角时处理uv
        // 收集2*2邻域
        uint2 neighbors[4] = {
            uint2(DTid.x, DTid.y),
            uint2(min(DTid.x + 1, dstWidth - 1), DTid.y),
            uint2(DTid.x, min(DTid.y + 1, dstHeight - 1)),
            uint2(min(DTid.x + 1, dstWidth - 1), min(DTid.y + 1, dstHeight - 1))
        };

        float4 avg = float4(0.0f, 0.0f, 0.0f, 0.0f);
        for(int i=0;i<4;i++){
            float2 srcXY = float2(
                (neighbors[i].x + 0.5) / dstWidth,
                (neighbors[i].y + 0.5) / dstHeight
            );
            float4 t_rgba = InputTexture.SampleLevel(bilinearSampler, srcXY, 0);
            avg += t_rgba;
        }
        // 计算UV值
        avg /= 4.0;
        float avgU = dot(avg, UCoeff) + 0.5;
        float avgV = dot(avg, VCoeff) + 0.5;
        avgU = avgU * 224.0/255.0 + 16.0/255.0;
        avgV = avgV * 224.0/255.0 + 16.0/255.0;

        OutputUV[uint2(DTid.x / 2, DTid.y / 2)] = float2(
            saturate(avgU), 
            saturate(avgV)
        );
    }

    // if(DTid.x % 2 == 0 && DTid.y % 2 == 0){ // 当线程位于像素块坐上角时处理uv
    //     // 根据像素块rgba数据计算uv值, 并将其写入UV平面
    //     float avgU = dot(rgba, UCoeff) + 0.5;
    //     float avgV = dot(rgba, VCoeff) + 0.5;

    //     // 映射范围到[16/255, 240/255]
    //     avgU = saturate(avgU);
    //     avgV = saturate(avgV);
    //     avgU = avgU * 224.0/255.0 + 16.0/255.0;
    //     avgV = avgV * 224.0/255.0 + 16.0/255.0;

    //     OutputUV[uint2(DTid.x / 2, DTid.y / 2)] = float2(
    //         saturate(avgU), 
    //         saturate(avgV)
    //     );
    // }

    // 只有当前线程位置全为偶数即在像素块左上角时，计算当前像素块的uv值
    // if((DTid.x % 2 == 0) && (DTid.y % 2 == 0)) {
    //     // 获取2x2邻域像素
    //     // float4 topLeft = rgba;
    //     // float4 topRight = InputTexture[uint2(min(DTid.x + 1, width - 1), DTid.y)];
    //     // float4 bottomLeft = InputTexture[uint2(DTid.x, min(DTid.y + 1, height - 1))];
    //     // float4 bottomRight = InputTexture[uint2(min(DTid.x + 1, width - 1), min(DTid.y + 1, height - 1))];

    //     // 获取当前块对应的四个采样位置
    //     float2 uvTopLeft     = float2((DTid.x + 0.5) * inverseScale.x, (DTid.y + 0.5) * inverseScale.y);
    //     float2 uvTopRight    = float2((DTid.x + 1.5) * inverseScale.x, (DTid.y + 0.5) * inverseScale.y);
    //     float2 uvBottomLeft  = float2((DTid.x + 0.5) * inverseScale.x, (DTid.y + 1.5) * inverseScale.y);
    //     float2 uvBottomRight = float2((DTid.x + 1.5) * inverseScale.x, (DTid.y + 1.5) * inverseScale.y);

    //     // 采样四个点的RGB值
    //     float4 rgbaTL = InputTexture.SampleLevel(bilinearSampler, uvTopLeft, 0);
    //     float4 rgbaTR = InputTexture.SampleLevel(bilinearSampler, uvTopRight, 0);
    //     float4 rgbaBL = InputTexture.SampleLevel(bilinearSampler, uvBottomLeft, 0);
    //     float4 rgbaBR = InputTexture.SampleLevel(bilinearSampler, uvBottomRight, 0);

    //     // 根据像素块rgba数据计算uv值, 并将其写入UV平面
    //     float avgU = dot((rgbaTL + rgbaTR + rgbaBL + rgbaBR) / 4.0, UCoeff) + 0.5;
    //     float avgV = dot((rgbaTL + rgbaTR + rgbaBL + rgbaBR) / 4.0, VCoeff) + 0.5;

    //     // 映射范围到[16/255, 240/255]
    //     avgU = saturate(avgU);
    //     avgV = saturate(avgV);
    //     avgU = avgU * 224.0/255.0 + 16.0/255.0;
    //     avgV = avgV * 224.0/255.0 + 16.0/255.0;

    //     OutputUV[uint2(DTid.x / 2, DTid.y / 2)] = float2(
    //         saturate(avgU), 
    //         saturate(avgV)
    //     );
    // }
}
