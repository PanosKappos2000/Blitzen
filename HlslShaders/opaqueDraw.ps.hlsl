struct PSOutput
{
    float4 color : SV_TARGET;
};

Texture2D textures[5000] : register(t8);
SamplerState texSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uvMapping : TEXCOORD0;
};

cbuffer textureTags : register(b2)
{
    uint albedoTag;
    uint normalTag;
    uint specularTag;
    uint emissiveTag;
    uint materialId;
};

PSOutput main(VSOutput input) : SV_TARGET
{
    PSOutput output;

    output.color = textures[0].Sample(texSampler, input.uvMapping);
    //output.color = float4(0.6, 0.1, 0, 1.f);

    return output;
}