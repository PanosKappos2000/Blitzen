struct Vertex
{
    float3 position; 
    uint2 uvCoordU, uvCoordV;     
    uint normalX, normalY, normalZ, normalW; 
    uint tangentX, tangentY, tangentZ, tangentW;
};

StructuredBuffer<Vertex> vertexBuffer : register(t0);

struct Transform
{
    float3 position;
    float scale;
    float4 orientation;
};

StructuredBuffer<Transform> transformBuffer : register(t3);

cbuffer ViewData : register(b0)  
{
    float4x4 view;
    float4x4 projectionView;
    float3 position;

    float frustumRight;
    float frustumLeft;
    float frustumTop;
    float frustumBottom;

    float proj0;
    float proj5;

    float zNear;
    float zFar;

    float pyramidWidth;
    float pyramidHeight;

    float lodTarget;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

float3 RotateQuat(float3 v, float4 quat)
{
    return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

// The main vertex shader function
VSOutput main(uint vertexIndex : SV_VertexID)
{
    Vertex vtx = vertexBuffer[vertexIndex];
    VSOutput output;
    float3 modelPos = RotateQuat(vtx.position, transformBuffer[0].orientation) * transformBuffer[0].scale + transformBuffer[0].position;
    output.position = mul(projectionView, (float4(modelPos, 1.0f))); 

    return output;
}