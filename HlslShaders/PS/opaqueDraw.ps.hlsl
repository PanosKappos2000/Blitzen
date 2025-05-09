struct PSOutput
{
    float4 color : SV_TARGET;
};

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
StructuredBuffer<Material> materialBuffer : register(t5);

//Texture2D<float4> tex : register(t8);
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
    Material mat = materialBuffer[input.materialId];

    float4 albedoMap = float4(0.5, 0.5, 0.5, 1);
    if(mat.albedoTag != 17)
    {
        Texture2D<float4> albedo = ResourceDescriptorHeap[mat.albedoTag];
        albedoMap = albedo.Sample(texSampler, input.uvMapping);
    }
    
    float3 normalMap = float3(0, 0, 1);
    if(mat.normalTag != 17)
    {
        Texture2D<float4> normal = ResourceDescriptorHeap[mat.normalTag];
        normalMap = normal.Sample(texSampler, input.uvMapping).rgb * 2 - 1;
    }

    float3 emissiveMap = float3(0.0, 0.0, 0.0);
    if(mat.emissiveTag != 17)
    {
        Texture2D<float4> emissive = ResourceDescriptorHeap[mat.emissiveTag];
        emissiveMap = emissive.Sample(texSampler, input.uvMapping).rgb;
    }

    output.color = float4(albedoMap.rgb /** sqrt(ndotl + 0.05)*/ + emissiveMap, albedoMap.a);

    return output;
}