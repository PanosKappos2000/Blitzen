#define OPAQUE_STATIC_CULL

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/staticCull.hlsl"
#include "../Headers/cullOut.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../../Resources/blitShaderShared.h"

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x + BLIT_OPAQUE_STATIC_RENDER_OFFSET;
    
    // Early return if it's out of bounds
    if (objId >= objCount + BLIT_OPAQUE_STATIC_RENDER_OFFSET)
    {
        return;
    }
    
    Render obj = ssbo_Renders[objId];
    Surface surface = ssbo_Surfaces[obj.surfaceId];

    // Bounding sphere to view coordinates
    float3 center = mul(viewMatrix, float4(ssbo_BoundingSpheres[objId].center, 1)).xyz;
    float radius = ssbo_BoundingSpheres[objId].radius;

    // Frustum culling
    if (!FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar))
    {
        return;
    }
    
    // If the render object get past culling, lod selection is done and draw command is added
    uint lodId = LODSelection(center, radius, ssbo_Transforms[obj.transformId].scale, lodTarget, surface.lodOffset, surface.lodCount);
    PrepareDrawCmd(lodId, objId);
}