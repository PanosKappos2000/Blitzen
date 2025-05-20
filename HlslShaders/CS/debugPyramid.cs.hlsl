Texture2D<float> tex_depthTexture : register(t0);
SamplerState smp_pSampler : register(s0);
RWBuffer<float4> rwb_outputColor : register(u0);

[numthreads(1, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    float depthValue = tex_depthTexture.Load(int3(dispatchThreadID.xy, 0));  // Assuming mip level 0
    
    rwb_outputColor[0] = float4(depthValue, depthValue, depthValue, 1.0f);
}