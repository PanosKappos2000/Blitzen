struct PSOutput
{
    float4 color : SV_TARGET;
};

#include "../Headers/psBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

//#define NO_PS_TEST

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

    float3 bitangent = cross(input.normal, input.tangent.xyz) * input.tangent.w;
    float3 finalTangent = input.tangent.xyz - dot(input.tangent.xyz, input.normal) * input.normal;
	float3 nrm = normalize(normalMap.r * finalTangent + normalMap.g * bitangent + normalMap.b * input.normal);
    float3 sunDirection = normalize(float3(-1, 1, -1));
	float ndotl = max(dot(nrm, sunDirection), 0.0);

    output.color = float4(albedoMap.rgb * sqrt(ndotl + 0.05) + emissiveMap, albedoMap.a);
    return output;
    
#endif
}