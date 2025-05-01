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
    VSOutput output;
    float3 modelPos = RotateQuat(vtx.position, transformBuffer[0].orientation) * transformBuffer[0].scale + transformBuffer[0].position;
    output.position = mul(projectionView, (float4(modelPos, 1.0f))); 

    return output;
}