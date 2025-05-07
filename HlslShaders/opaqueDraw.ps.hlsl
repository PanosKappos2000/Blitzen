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

PSOutput main() : SV_TARGET
{
    PSOutput output;
    output.color = float4(1.0f, 0.0f, 0.0f, 1.0f); 
    return output;
}