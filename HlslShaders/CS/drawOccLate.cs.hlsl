#define DRAW_CULL_OCCLUSION
#define DRAW_CULL_OCCLUSION_LATE

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

    Render obj = renderBuffer[objId];
    Surface surface = surfaceBuffer[obj.surfaceId];
    Transform transform = transformBuffer[obj.transformId];

    // Promotes the bounding sphere's center to model and the view coordinates (frustum culling will be done on view space)
    float3 center = RotateQuat(surface.center, transform.orientation) * transform.scale + transform.position;
    center = mul(viewMatrix, float4(center, 1)).xyz;
    float radius = surface.radius * transform.scale;

    // Frustum culling
    bool visible = FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar);

    // Occlusion culling
    if (visible)
	{
		float4 aabb;
		if (ProjectSphere(center, radius, zNear, proj0, proj5, aabb))
		{
			visible = visible && OcclusionCheck(aabb, depthPyramid, dpSampler, pyramidWidth, pyramidHeight, center, radius, zNear);
		}
	}

    if(visible && drawVisibilityBuffer[objId] == 0)
    {
        uint lodId = LODSelection(center, radius, transform.scale, lodTarget, surface.lodOffset, surface.lodCount);

        // Command count
        uint cmdId;
        InterlockedAdd(drawCountBuffer[0], 1, cmdId);

        // Render object id constant
        drawCmdBuffer[cmdId].objId = objId;

        // Vertices
        drawCmdBuffer[cmdId].indexCount = lodBuffer[lodId].indexCount;
        drawCmdBuffer[cmdId].indexOffset = lodBuffer[lodId].indexOffset;
        drawCmdBuffer[cmdId].vertOffset = 0; // Already added to the index buffer

        // Instances
        drawCmdBuffer[cmdId].instCount = 1;
        drawCmdBuffer[cmdId].insOffset = 0;
    }

    // Sets next frame visibility
    drawVisibilityBuffer[objId] = visible ? 1 : 0;
}