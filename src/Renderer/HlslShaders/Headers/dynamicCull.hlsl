struct WVTransform
{
    float3 position;
    float3 euler;
    uint movementFlags;
    uint padding1;
};
RWStructuredBuffer<WVTransform> rwssbo_HostTransform : register(u14);

StructuredBuffer<float> ssbo_TerrainHeight : register(t3);

cbuffer ObjCountConstant : register(b2)
{
    uint objCount;
};