#define OPAQUE_DYNAMIC_CULL
#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/dynamicCull.hlsl"
#include "../Headers/occlusionCull.hlsl"
#include "../Headers/cullOut.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/wvCollision.hlsl"
#include "../../Resources/blitShaderShared.h"


[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x + BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET;
    if (objId > workCount + BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET)
    {
        return;
    }

    // Original resident position
    float3 position = ssbo_Transforms[objId].position;

    // Do not test residents outside of this grid
    // This check might be removed later as objects outside the grid should not be loaded at all
    if ((int) position.x > maxBounds || (int) position.x < minBounds || (int) position.z > maxBounds || (int) position.z < minBounds)
    {
        return;
    }

    // Switches position to be inside the grid's origin. 
    // If the grid is properly transformed for where it is placed in the world, this becomes unnecessary
    position -= float3(float(minBounds), float(minBounds), float(minBounds));

    // Check for unexpected mistake where the position has somehow ended up outside the grid
    if (position.x < 0.f || position.x > BLIT_COLLISION_GRID_EXTENT || position.z < 0.f || position.z > BLIT_COLLISION_GRID_EXTENT)
    {
        return;
    }

    // Divides the position on the x and z axis by the cell's extent, to find in which cell it belongs to
    uint cellPosX = (position.x / BLIT_COLLISION_GRID_CELL_EXTENT) < BLIT_COLLISION_GRID_CELL_FLAT_COUNT ?
                uint(position.x / BLIT_COLLISION_GRID_CELL_EXTENT) : BLIT_COLLISION_GRID_CELL_FLAT_COUNT - 1;
    uint cellPosZ = (position.z / BLIT_COLLISION_GRID_CELL_EXTENT) < BLIT_COLLISION_GRID_CELL_FLAT_COUNT ?
                uint(position.z / BLIT_COLLISION_GRID_CELL_EXTENT) : BLIT_COLLISION_GRID_CELL_FLAT_COUNT - 1;

    // Retrieves flat index by adding the x position to the z position multiplied by the cell count on each axis (flat count)
    uint cellIndex = cellPosX + cellPosZ * BLIT_COLLISION_GRID_CELL_FLAT_COUNT;
    
    // Protects against error once more
    if (cellIndex >= BLIT_COLLISION_GRID_CELL_COUNT)
    {
        return;
    }
    
    // At this stage the collider count is incremented
    InterlockedAdd(rw_Cells[cellIndex].colliderCount, 1);
    rwssbo_HostTransform[objId].cellID = cellIndex;
}