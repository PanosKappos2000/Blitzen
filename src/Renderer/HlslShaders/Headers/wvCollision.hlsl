cbuffer cb_MoveCount : register(b10)
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

StructuredBuffer<float4> ssbo_ColliderAMaxRad : register(t17);
StructuredBuffer<float4> ssbo_ColliderBMinType : register(t18);

RWBuffer<float4> rwssbo_TransformedColliderAMaxRad : register(u17);
RWBuffer<float4> rwssbo_TransformedColliderBMinType : register(u18);

RWBuffer<uint> rwssbo_CurrentColliderOffset : register(u19);