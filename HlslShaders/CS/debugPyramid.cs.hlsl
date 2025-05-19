Texture2D<float> depthTexture : register(t0);
SamplerState texSampler : register(s0);
RWBuffer<float4> outputColor : register(u0);

[numthreads(1, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float depthValue = depthTexture.Load(int3(dispatchThreadID.xy, 0));  // Assuming mip level 0
    
    outputColor[0] = float4(depthValue, depthValue, depthValue, 1.0f);
}