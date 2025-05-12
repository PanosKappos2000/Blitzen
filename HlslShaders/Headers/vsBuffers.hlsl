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