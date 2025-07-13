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
RWBuffer<float3> rw_ColliderFloat3AMax : register(u17);
RWBuffer<float3> rw_ColliderFloat3BMin : register(u18);
RWBuffer<float> rw_ColliderFloat : register(u19);