Texture2D<float4> depthIn : register(t0);
RWTexture2D<float4> depthOut : register(u0);
SamplerState pyramidSampler : register(s0);

cbuffer PyramidMip : register(b0)
{
    float2 imageSize;
    uint mipLevel;
};

// Define the workgroup size
[numthreads(32, 32, 1)] 
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float2 texCoords = (float2(dispatchThreadID.xy) + float2(0.5f, 0.5f)) / imageSize;

    float depth = depthIn.SampleLevel(pyramidSampler, texCoords, mipLevel).r;

    if (dispatchThreadID.x < imageSize.x - 1 && dispatchThreadID.y < imageSize.y - 1)
    {
        float depthRight = depthIn.SampleLevel(pyramidSampler, texCoords + float2(1.f, 0.f), mipLevel).r;
        float depthDown = depthIn.SampleLevel(pyramidSampler, texCoords + float2(0.f, 1.f), mipLevel).r;
        
        depth = min(depth, min(depthRight, depthDown));
    }

    depthOut[texCoords] = depth;
}