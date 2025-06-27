#define OPAQUE_STATIC_CULL
#define HI_Z_MAP_OCCLUSION

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/staticCull.hlsl"
#include "../Headers/occdp.hlsl"
#include "../Headers/occlusionCull.hlsl"
#include "../Headers/cullOut.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/cpuShared.h"

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x + BLIT_OPAQUE_STATIC_RENDER_OFFSET;
    // Early return if it's out of bounds
    if(objId >= objCount)
    {
        return;
    }

    Render obj = ssbo_Renders[objId];
    Surface surface = ssbo_Surfaces[obj.surfaceId];
    Transform transform = ssbo_Transforms[obj.transformId];

    // Bounding sphere to view coordinates
    float3 center = mul(viewMatrix, float4(ssbo_BoundingSpheres[objId].center, 1)).xyz;
    float radius = ssbo_BoundingSpheres[objId].radius;

    // Frustum culling
    bool visible = FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar);

    // Occlusion culling
    if (visible)
	{
		float4 aabb = float4(0, 0, 0, 0);
		if (ProjectSphere(center, radius, zNear, proj0, proj5, aabb))
		{
			visible = visible && OcclusionCheck(aabb, pyramidWidth, pyramidHeight, center, radius, zNear);
		}
	}

    if(visible && rwssbo_DrawVisibilityBuffer[objId] == 0)
    {
        uint lodId = LODSelection(center, radius, transform.scale, lodTarget, surface.lodOffset, surface.lodCount);

        PrepareDrawCmd(lodId, objId);
    }

    // Sets next frame visibility
    rwssbo_DrawVisibilityBuffer[objId] = visible ? 1 : 0;
}