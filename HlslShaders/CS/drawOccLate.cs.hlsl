#define DRAW_CULL_OCCLUSION
#define HI_Z_MAP_DRAW_OCCLUSION

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

cbuffer ObjCountConstant: register (b2)
{
    uint objCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x;
    // Early return if it's out of bounds
    if(objId >= objCount)
    {
        return;
    }

    Render obj = ssbo_Renders[objId];
    Surface surface = ssbo_Surfaces[obj.surfaceId];
    Transform transform = ssbo_Transforms[obj.transformId];

    // Promotes the bounding sphere's center to model and the view coordinates (frustum culling will be done on view space)
    float3 center = RotateQuat(surface.center, transform.orientation) * transform.scale + transform.position;
    center = mul(viewMatrix, float4(center, 1)).xyz;
    float radius = surface.radius * transform.scale;

    // Frustum culling
    bool visible = FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar);

    // Occlusion culling
    if (visible)
	{
		float4 aabb = float4(0, 0, 0, 0);
		if (ProjectSphere(center, radius, zNear, proj0, proj5, aabb))
		{
			visible = visible && OcclusionCheck(aabb, tex_HiZMap, pyramidWidth, pyramidHeight, center, radius, zNear);
		}
	}

    if(visible && rwssbo_DrawVisibilityBuffer[objId] == 0)
    {
        uint lodId = LODSelection(center, radius, transform.scale, lodTarget, surface.lodOffset, surface.lodCount);

        // Command count
        uint cmdId;
        InterlockedAdd(rwb_DrawCmdCounter[0], 1, cmdId);

        // Render object id constant
        ssbo_DrawCmd[cmdId].objId = objId;

        // Vertices
        ssbo_DrawCmd[cmdId].indexCount = ssbo_LODs[lodId].indexCount;
        ssbo_DrawCmd[cmdId].indexOffset = ssbo_LODs[lodId].indexOffset;
        ssbo_DrawCmd[cmdId].vertOffset = 0; // Already added to the index buffer

        // Instances
        ssbo_DrawCmd[cmdId].instCount = 1;
        ssbo_DrawCmd[cmdId].insOffset = 0;
    }

    // Sets next frame visibility
    rwssbo_DrawVisibilityBuffer[objId] = visible ? 1 : 0;
}