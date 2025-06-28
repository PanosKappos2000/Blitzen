StructuredBuffer<float3> ssbo_VtxPositions : register(t9);
StructuredBuffer<float4> ssbo_VtxNormals : register(t10);
StructuredBuffer<float4> ssbo_VtxTangents : register(t11);
StructuredBuffer<float2> ssbo_VtxTexCoords : register(t12);

#ifdef OPAQUE_STATIC
cbuffer ObjId : register(b3)
{
    uint objId;
};
#endif

#ifdef OPAQUE_DYNAMIC
cbuffer ObjId : register(b4)
{
    uint objId;
};
#endif

#ifdef INSTANCING
cbuffer RESOURCE_ID : register(b3)
{
    uint instanceOffset;
    uint resourceID;
};
#endif

#ifdef CLUSTER

#endif

#ifdef TRANSPARENT

#endif

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    uint materialId : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};