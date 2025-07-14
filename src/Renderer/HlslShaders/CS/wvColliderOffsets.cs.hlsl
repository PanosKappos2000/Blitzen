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
    
    uint cellIndex = rwssbo_HostTransform[objId].cellID;
    
    uint IDX;
    // At this stage the collider count is incremented
    InterlockedAdd(rw_Cells[cellIndex].colliderCount, 1, IDX);
    
    // Most of these checks will be removed
    if (rw_Cells[cellIndex].colliderOffset + IDX > workCount + BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET)
    {
        return;
    }
    
    // Add the resident ID as a collider ID in the correct offset according to its grid.
    rw_ColliderIndices[rw_Cells[cellIndex].colliderOffset + IDX] = objId;
}