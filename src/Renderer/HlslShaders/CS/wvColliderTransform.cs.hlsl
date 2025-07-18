#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/dynamicCull.hlsl"
#include "../Headers/wvCollision.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../../Resources/blitShaderShared.h"

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint objID = dispatchThreadID.x;
    if (objID >= workCount)
    {
        return;
    }
    
    uint type = (uint) ssbo_ColliderBMinType[objID].w;
    if (type == BlitzenColliderTypeSphere)
    {
        rwssbo_TransformedColliderAMaxRad[objID].xyz = RotateQuat(ssbo_ColliderAMaxRad[objID].xyz, ssbo_Transforms[objID].orientation) * ssbo_Transforms[objID].scale + ssbo_Transforms[objID].position;
        rwssbo_TransformedColliderAMaxRad[objID].w = ssbo_ColliderAMaxRad[objID].w * ssbo_Transforms[objID].scale;
    }
    else if (type == BlitzenColliderTypeAABB)
    {
        rwssbo_TransformedColliderAMaxRad[objID].xyz = RotateQuat(ssbo_ColliderAMaxRad[objID].xyz, ssbo_Transforms[objID].orientation) * ssbo_Transforms[objID].scale + ssbo_Transforms[objID].position;
        rwssbo_TransformedColliderBMinType[objID].xyz = RotateQuat(ssbo_ColliderBMinType[objID].xyz, ssbo_Transforms[objID].orientation) * ssbo_Transforms[objID].scale + ssbo_Transforms[objID].position;
    }
    else if (type == BlitzenColliderTypeCapsule)
    {
        rwssbo_TransformedColliderAMaxRad[objID].xyz = RotateQuat(ssbo_ColliderAMaxRad[objID].xyz, ssbo_Transforms[objID].orientation) * ssbo_Transforms[objID].scale + ssbo_Transforms[objID].position;
        rwssbo_TransformedColliderBMinType[objID].xyz = RotateQuat(ssbo_ColliderBMinType[objID].xyz, ssbo_Transforms[objID].orientation) * ssbo_Transforms[objID].scale + ssbo_Transforms[objID].position;
        rwssbo_TransformedColliderAMaxRad[objID].w = ssbo_ColliderAMaxRad[objID].w * ssbo_Transforms[objID].scale;
    }
}