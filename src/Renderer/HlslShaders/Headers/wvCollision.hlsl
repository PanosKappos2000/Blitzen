cbuffer cb_MoveCount : register(b10)
{
    uint workCount;
    uint maxBounds;
    uint minBounds;
};

cbuffer cb_HittingColliderIDX : register(b11)
{
    uint hitterID;
}

struct GRID_CELL
{
    uint staticColliderOffset;
    uint staticColliderCount;
    uint dynamicColliderOffset;
    uint dynamicColliderCount;
};
RWStructuredBuffer<GRID_CELL> rw_Cells : register(u15);

RWBuffer<uint> rw_ColliderIndices : register(u16);

RWBuffer<uint> rwssbo_CurrentColliderOffset : register(u19);