#include "../Headers/wvCollision.hlsl"
#include "../../Resources/blitShaderShared.h"

[numthreads(1, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    rwssbo_CurrentColliderOffset[0] = 0;
}

