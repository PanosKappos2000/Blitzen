struct Movement
{
    float3 velocity;
    float3 rotation;
    uint padding0;
    uint padding1;
};
StructuredBuffer<Movement> ssbo_Movements : register(t3);

cbuffer ObjCountConstant : register(b2)
{
    uint objCount;
};