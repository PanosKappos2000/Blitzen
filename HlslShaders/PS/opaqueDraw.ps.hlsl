struct PSOutput
{
    float4 color : SV_TARGET;
};

struct Material
{
    float4 diffuseColor;
    float shininess;

    uint albedoTag;

    uint normalTag;

    uint specularTag;

    uint emissiveTag;

    uint materialId;
};
StructuredBuffer<Material> materialBuffer : register(t5);

Texture2D<float4> tex : register(t8);
SamplerState texSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uvMapping : TEXCOORD0;
    uint materialId : TEXCOORD1;
};

PSOutput main(VSOutput input)
{
    PSOutput output;

    //output.color = tex[materialBuffer[input.materialId].albedoTag].Sample(texSampler, input.uvMapping);
    output.color = float4(0.6, 0.1, 0, 1.f);

    return output;
}