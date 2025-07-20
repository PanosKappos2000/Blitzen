#define COLLIDER_CULL

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/dynamicCull.hlsl"
#include "../Headers/colliders.hlsl"
#include "../../Resources/blitShaderShared.h"

#define COLLIDER_SHAPE_FIRST_LOD        0

[numthreads(64, 1, 1)]
void csMain(uint dispatchGroupID : SV_DispatchThreadID)
{
    uint objID = dispatchGroupID.x;
    if(objID > objCount)
    {
        return;
    }
    
    uint type = (uint) ssbo_ColliderBMinType[objID].w;
    if(type >= BlitzenColliderTypeMax)
    {
        // Handle false type
        return;
    }
    
    Surface surface = ssbo_Surfaces[type + BLIT_HLSL_COLLIDER_RESOURCE_OFFSET];

    // Bounding sphere to view coordinates
    float3 center = mul(viewMatrix, float4(ssbo_BoundingSpheres[objID].center, 1)).xyz;
    float radius = ssbo_BoundingSpheres[objID].radius;

    // Frustum culling
    if (!FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar))
    {
        return;
    }
    
    // No LODs
    PrepareDrawCmd(COLLIDER_SHAPE_FIRST_LOD, objID);
}