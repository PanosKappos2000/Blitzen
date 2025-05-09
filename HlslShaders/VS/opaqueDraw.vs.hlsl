#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

struct Vertex
{
    float3 position;
    float mappingU, mappingV;
    uint normals, tangents;
    uint padding0;
};
StructuredBuffer<Vertex> vertexBuffer : register(t0);

cbuffer ObjId : register(b1)
{
    uint objId;
}

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uvMapping : TEXCOORD0;
    uint materialId : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

// The main vertex shader function
VSOutput main(uint vertexIndex : SV_VERTEXID)
{
    Vertex vtx = vertexBuffer[vertexIndex];
    Render obj = renderBuffer[objId];
    float4 orientation = transformBuffer[obj.transformId].orientation;
    VSOutput output;

    // Position
    float3 modelPos = RotateQuat(vtx.position, orientation) * transformBuffer[obj.transformId].scale + transformBuffer[obj.transformId].position;
    output.position = mul(projectionView, (float4(modelPos, 1.0f))); 
    
    // Material index
    output.materialId = surfaceBuffer[obj.surfaceId].materialId;

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