struct PSOutput
{
    float4 color : SV_TARGET;
};

#include "../Headers/cpuShared.h"
#include "../Headers/psBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

//#define NO_PS_TEST

Texture2D<float4> tex_Textures[BLIT_MAX_WORLD_TEXTURE_RESOURCES] : register(t15);

PSOutput main(VSOutput input)
{
#ifdef NO_PS_TEST
    
    PSOutput output;
    output.color = float4(input.tangent.xyz, 1);
    return output;
    
#else
    
    PSOutput output;
    Material mat = ssbo_MaterialBuffer[input.materialId];
    
    float4 albedoMap = float4(0.5, 0.5, 0.5, 1);
    if(mat.albedoTag != 0)
    {
        Texture2D<float4> albedo = tex_Textures[NonUniformResourceIndex(mat.albedoTag)];
        albedoMap = albedo.Sample(smp_textureSampler, input.texCoord);
    }
    
    if (albedoMap.a < 0.5)
    {
        discard;
    }
    
    float3 normalMap = float3(0, 0, 1);
    if(mat.normalTag != 0)
    {
        Texture2D<float4> normal = tex_Textures[NonUniformResourceIndex(mat.normalTag)];
        normalMap = normal.Sample(smp_textureSampler, input.texCoord).rgb * 2 - 1;
    }
    
    float3 emissiveMap = float3(0.0, 0.0, 0.0);
    if(mat.emissiveTag != 0)
    {
        Texture2D<float4> emissive = tex_Textures[NonUniformResourceIndex(mat.emissiveTag)];
        emissiveMap = emissive.Sample(smp_textureSampler, input.texCoord).rgb;
    }

    float3 directionalLighting = CalculateDirectionalLighting(input.normal, input.tangent, normalMap);

    output.color = float4(albedoMap.rgb * sqrt(directionalLighting + 0.05) + emissiveMap, albedoMap.a);
    
    return output;
    
#endif
}