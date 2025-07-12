struct Movement
{
    float3 velocity;
    float3 rotation;
    uint movementFlags;
    uint padding1;
};
RWStructuredBuffer<Movement> rwssbo_HostTransform : register(u14);

struct WVMovement
{
    float velocityZ;
    float velocityX;
    float velocityY;
    float rotationPitch;
    float rotationYaw;
    float rotationRoll;
    uint padding0;
    uint padding1;
};
RWStructuredBuffer<WVMovement> rwssbo_HostLogicMovment : register(u17);

StructuredBuffer<float> ssbo_TerrainHeight : register(t3);

cbuffer ObjCountConstant : register(b2)
{
    uint objCount;
};