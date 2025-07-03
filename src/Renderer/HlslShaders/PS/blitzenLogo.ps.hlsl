Texture2D<float4> blitzenTexture : register(t0);
SamplerState smp_textureSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

struct PSOutput
{
    float4 color : SV_TARGET;
};

PSOutput main(PSInput input) : SV_TARGET
{
    PSOutput output;
    output.color = blitzenTexture.Sample(smp_textureSampler, input.uv);
    return output;
}