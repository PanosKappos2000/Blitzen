#define DRAW_CULL_OCCLUSION
#define DRAW_CULL_OCCLUSION_LATE

Texture2D<float4> tex_depthIn : register(t0);
RWTexture2D<float4> rwtex_depthOut : register(u0);

cbuffer PyramidMip : register(b0)
{
    uint mipLevel;
};

// Define the workgroup size
[numthreads(32, 32, 1)] 
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 texCoords = uint2(dispatchThreadID.xy) << 1;

    float4 texels;
    texels.r = tex_depthIn.Load(uint3(texCoords, mipLevel)).r;
    texels.g = tex_depthIn.Load(uint3(texCoords + uint2(1, 0), mipLevel)).r;
    texels.b = tex_depthIn.Load(uint3(texCoords + uint2(0, 1), mipLevel)).r;
    texels.a = tex_depthIn.Load(uint3(texCoords + uint2(1, 1), mipLevel)).r;

    texels.r = min(texels.r, min(texels.g, min(texels.b, texels.a)));
    
    rwtex_depthOut[dispatchThreadID.xy] = float4(texels.r, texels.r, texels.r, texels.r);
}