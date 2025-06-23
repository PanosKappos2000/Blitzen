#define DRAW_CULL_OCCLUSION

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/cpuShared.h"

cbuffer ObjCountConstant: register (b1)
{
    uint objCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x + BLIT_OPAQUE_STATIC_RENDER_OFFSET;
    
    // Early return if it's out of bounds
    if (objId >= objCount + BLIT_OPAQUE_STATIC_RENDER_OFFSET)
    {
        return;
    }

    if(rwssbo_DrawVisibilityBuffer[objId] == 0)
    {
        return;
    }

    Render obj = ssbo_Renders[objId];
    Surface surface = ssbo_Surfaces[obj.surfaceId];
    Transform transform = ssbo_Transforms[obj.transformId];

    // Promotes the bounding sphere's center to model and the view coordinates (frustum culling will be done on view space)
    float3 center = RotateQuat(ssbo_BoundingSpheres[objId].center, transform.orientation) * transform.scale + transform.position;
    center = mul(viewMatrix, float4(center, 1)).xyz;
    float radius = ssbo_BoundingSpheres[objId].radius * transform.scale;

    // Frustum culling
    bool visible = FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar);

    if(visible)
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
}