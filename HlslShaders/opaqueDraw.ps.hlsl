struct PSOutput
{
    float4 color : SV_TARGET;
};

struct Material
{
    // Not using these 2 right now, might remove them when I'm not bored, 
    // but they (or something similar) will be used in the future
    float4 diffuseColor;
    float shininess;

    uint albedoTag;

    uint normalTag;

    uint specularTag;

    uint emissiveTag;

    // TODO: I need to try removing this, it's a waste of space
    uint materialId;
};
StructuredBuffer<Material> materialBuffer : register(t1);

Texture2D textures[1000] : register(t8);
SamplerState texSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uvMapping : TEXCOORD0;
    uint materialId : TEXCOORD1;
};


PSOutput main(VSOutput input) : SV_TARGET
{
    PSOutput output;

    //output.color = textures[materialBuffer[input.materialId].albedoTag].Sample(texSampler, input.uvMapping);
    output.color = float4(0.6, 0.1, 0, 1.f);

    return output;
}