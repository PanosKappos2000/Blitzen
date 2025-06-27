StructuredBuffer<float3> ssbo_VtxPositions : register(t9);
StructuredBuffer<float4> ssbo_VtxNormals : register(t10);
StructuredBuffer<float4> ssbo_VtxTangents : register(t11);
StructuredBuffer<float2> ssbo_VtxTexCoords : register(t12);

cbuffer ObjId : register(b3)
{
    uint objId;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    uint materialId : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};