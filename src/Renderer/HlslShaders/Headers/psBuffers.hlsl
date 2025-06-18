struct Material
{
    uint albedoTag;
    uint normalTag;
    uint specularTag;
    uint emissiveTag;

    uint materialId;
    uint padding0;
    uint padding1;
    uint padding2;
};
StructuredBuffer<Material> ssbo_MaterialBuffer : register(t7);

SamplerState smp_textureSampler : register(s0);
Texture2D<float4> tex_Textures[5000] : register(t8);


struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    uint materialId : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};