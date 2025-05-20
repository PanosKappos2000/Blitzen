#include "../Headers/vsBuffers.hlsl"
#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

// The main vertex shader function
VSOutput main(uint vertexIndex : SV_VERTEXID)
{
    Vertex vtx = ssbo_Vertices[vertexIndex];
    Render obj = ssbo_Renders[objId];
    float4 orientation = ssbo_Transforms[obj.transformId].orientation;
    VSOutput output;

    // Position
    float3 modelPos = RotateQuat(vtx.position, orientation) * ssbo_Transforms[obj.transformId].scale + ssbo_Transforms[obj.transformId].position;
    output.position = mul(projectionView, (float4(modelPos, 1.0f))); 
    
    // Material index
    output.materialId = ssbo_Surfaces[obj.surfaceId].materialId;

    // Normals(unpacking required)
    float3 unpackedNormal = UnpackNormals(vtx.normals);
    output.normal = RotateQuat(unpackedNormal, orientation);

    // Tangents(unpacking required)
    float4 unpackedTangent = UnpackTangents(vtx.tangents);
    output.tangent.xyz = RotateQuat(unpackedTangent.xyz, orientation);
    output.tangent.w = unpackedTangent.w;

    // Texture coordinates
    output.uvMapping = float2(vtx.mappingU, vtx.mappingV);

    return output;
}