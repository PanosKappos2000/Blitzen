#define OPAQUE_DYNAMIC

#include "../Headers/vsBuffers.hlsl"
#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

// The main vertex shader function
VSOutput main(uint vertexIndex : SV_VERTEXID)
{
    Render obj = ssbo_Renders[objId];
    float4 orientation = ssbo_Transforms[obj.transformId].orientation;
    VSOutput output;

    // Position
    float3 modelPos = RotateQuat(ssbo_VtxPositions[vertexIndex], orientation) * ssbo_Transforms[obj.transformId].scale + ssbo_Transforms[obj.transformId].position;
    output.position = mul(projectionView, (float4(modelPos, 1.0f)));
    
    // Material index
    output.materialId = ssbo_Surfaces[obj.surfaceId].materialId;
    
    // Tex coords
    output.texCoord = ssbo_VtxTexCoords[vertexIndex];

    // Normal
    output.normal = RotateQuat(ssbo_VtxNormals[vertexIndex].xyz, orientation);

    // Tangent
    output.tangent.xyz = RotateQuat(ssbo_VtxTangents[vertexIndex].xyz, orientation);
    output.tangent.w = ssbo_VtxTangents[vertexIndex].w;

    return output;
}