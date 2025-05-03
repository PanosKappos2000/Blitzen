#include "HlslShaders/sharedBuffers.hlsl"

struct Vertex
{
    float3 position;

    uint mappingU;
    uint mappingV;

    uint normals;
    uint tangents;

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
};

float3 RotateQuat(float3 v, float4 quat)
{
    return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

// The main vertex shader function
VSOutput main(uint vertexIndex : SV_VERTEXID)
{
    Vertex vtx = vertexBuffer[vertexIndex];
    Render obj = renderBuffer[objId];

    VSOutput output;
    float3 modelPos = RotateQuat(vtx.position, transformBuffer[obj.transformId].orientation) * transformBuffer[obj.transformId].scale + transformBuffer[obj.transformId].position;
    output.position = mul(projectionView, (float4(modelPos, 1.0f))); 

    uint materialId = surfaceBuffer[obj.surfaceId].materialId;

    return output;
}