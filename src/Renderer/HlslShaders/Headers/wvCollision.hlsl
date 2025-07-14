cbuffer cb_MoveCount : register(b20)
{
    uint workCount;
    uint maxBounds;
    uint minBounds;
};

struct GRID_CELL
{
    uint colliderOffset;
    uint colliderCount;
};
RWStructuredBuffer<GRID_CELL> rw_Cells : register(u15);

RWBuffer<uint> rw_ColliderIndices : register(u16);
RWBuffer<float3> rwssbo_ColliderFloat3AMaxRad : register(u17);
RWBuffer<float3> rwssbo_ColliderFloat3BMinType : register(u18);