struct Movement
{
    float3 velocity;
    float3 rotation;
    uint movementFlags;
    uint padding1;
};
RWStructuredBuffer<Movement> rwssbo_HostTransform : register(u14);

StructuredBuffer<float> ssbo_TerrainHeight : register(t3);

cbuffer ObjCountConstant : register(b2)
{
    uint objCount;
};