#define INSTANCED_CULL
#define INSTANCING

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/instCull.hlsl"
#include "../Headers/cullOut.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/cpuShared.h"

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x;
    // Early return if it's out of bounds
    if (objId >= workCount)
    {
        return;
    }
    
    InstancedRender obj = ssbo_Instanced[objId];
    Transform transform = ssbo_Transforms[obj.transformId];

    float3 center = mul(viewMatrix, float4(ssbo_BoundingSpheres[objId].center, 1)).xyz;
    float radius = ssbo_BoundingSpheres[objId].radius;

    // Frustum culling
    if (!FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar))
    {
        return;
    }

    uint resourceId = rwssbo_DrawCmd[obj.drawCommandId].resourceId;
    uint lodOffset = ssbo_Surfaces[resourceId].lodOffset;
    uint lodCount = ssbo_Surfaces[resourceId].lodCount;
        
    uint lodId = LODSelection(center, radius, transform.scale, lodTarget, lodOffset, lodCount);
    InterlockedAdd(rwssbo_DrawCmd[obj.drawCommandId + lodId].instCount, 1);     
}