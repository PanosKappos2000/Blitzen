#include "../../Resources/blitShaderShared.h"
#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/colliders.hlsl"
#include "../Headers/vsBuffers.hlsl"

cbuffer cb_objID : register(b12)
{
    uint objID;
};

VSOutput main(uint vertexIndex : SV_VertexID)
{
    VSOutput output;
    
    uint type = (uint) ssbo_ColliderBMinType[objID].w;

    // Position
    float3 modelPos = ssbo_VtxPositions[vertexIndex] * ssbo_Transforms[objID].scale + ssbo_Transforms[objID].position;
    output.position = mul(projectionView, (float4(modelPos, 1.0f)));
    
    // Material index
    output.materialId = ssbo_Surfaces[type + BLIT_HLSL_COLLIDER_RESOURCE_OFFSET].materialId;
    
    // Tex coords
    output.texCoord = ssbo_VtxTexCoords[vertexIndex];

    // Normal
    output.normal = float3(0.f, 0.f, 0.f);

    // Tangent
    output.tangent.xyz = float3(0.f, 0.f, 0.f);
    output.tangent.w = ssbo_VtxTangents[vertexIndex].w;

    return output;
}