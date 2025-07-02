#define OPAQUE_DYNAMIC_CULL

/*
    THIS ONE IS JUST A DRAFT
*/
#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/dynamicCull.hlsl"
#include "../Headers/occlusionCull.hlsl"
#include "../Headers/cullOut.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../../Resources/blitShaderShared.h"

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
    uint dynamicColliderOffset;
    uint dynamicColliderCount;
};
RWStructuredBuffer<GRID_CELL> rw_Cells : register(u1);

RWBuffer<uint> rw_ColliderIndices : register(u2);

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x + BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET;
    if (objId > workCount + BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET)
    {
        return;
    }

    float3 position = ssbo_Transforms[objId].position;

    if ((int) position.x > maxBounds || (int) position.x < minBounds || (int) position.y > maxBounds || (int) position.y < minBounds || (int) position.z > maxBounds || (int) position.z < minBounds)
    {
        return;
    }

    position -= float3(float(minBounds), float(minBounds), float(minBounds));

    if (position.x < 0.f || position.x > BLIT_COLLISION_GRID_EXTENT || position.y < 0.f || position.x > BLIT_COLLISION_GRID_EXTENT || position.z < 0.f || position.z > BLIT_COLLISION_GRID_EXTENT)
    {
        return;
    }

    uint cellPosX = (position.x / BLIT_COLLISION_GRID_CELL_EXTENT) < BLIT_COLLISION_GRID_CELL_FLAT_COUNT ?
                uint(position.x / BLIT_COLLISION_GRID_CELL_EXTENT) : BLIT_COLLISION_GRID_CELL_FLAT_COUNT - 1;
    uint cellPosY = (position.y / BLIT_COLLISION_GRID_CELL_EXTENT) < BLIT_COLLISION_GRID_CELL_FLAT_COUNT ?
                uint(position.y / BLIT_COLLISION_GRID_CELL_EXTENT) : BLIT_COLLISION_GRID_CELL_FLAT_COUNT - 1;
    uint cellPosZ = (position.z / BLIT_COLLISION_GRID_CELL_EXTENT) < BLIT_COLLISION_GRID_CELL_FLAT_COUNT ?
                uint(position.z / BLIT_COLLISION_GRID_CELL_EXTENT) : BLIT_COLLISION_GRID_CELL_FLAT_COUNT - 1;

    uint cellIndex = cellPosX + cellPosY * BLIT_COLLISION_GRID_CELL_FLAT_COUNT + cellPosZ * BLIT_COLLISION_GRID_CELL_FLAT_COUNT * BLIT_COLLISION_GRID_CELL_FLAT_COUNT;
    if (cellIndex >= BLIT_COLLISION_GRID_CELL_COUNT)
    {
        return;
    }

    GRID_CELL grid = rw_Cells[cellIndex];
    if (grid.colliderCount >= BLIT_DYNAMIC_COLLIDER_COUNT_PER_GRID_CELL)
    {
        return;
    }

    uint colliderIndex = grid.dynamicColliderOffset + grid.dynamicColliderCount;
    if (colliderIndex >= BLIT_AVAILABLE_DYNAMIC_COLLIDER_SPACES)
    {
        return;
    }

    rw_ColliderIndices[colliderIndex] = objId;
    rw_Cells[cellIndex].dynamicColliderCount++;
}