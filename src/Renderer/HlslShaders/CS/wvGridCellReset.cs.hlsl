#include "../Headers/wvCollision.hlsl"
#include "../../Resources/blitShaderShared.h"

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x;
    if (objId > BLIT_COLLISION_GRID_CELL_COUNT)
    {
        return;
    }
    
    rw_Cells[objId].colliderCount = 0;
}