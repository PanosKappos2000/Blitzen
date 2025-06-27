#define INSTANCED_CULL

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/instCull.hlsl"
#include "../Headers/cullOut.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/cpuShared.h"

cbuffer ObjCountConstant: register (b1)
{
    uint objCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x;
    // Early return if it's out of bounds
    if (objId >= objCount)
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
        
        InterlockedAdd(rwb_DrawCmdCounter[lodId], 1);

        // Should set this stuff on load, there is 8 of them at best since this will only looks at one type of mesh.
        //rwssbo_InstDrawCmd[lodId].indexCount = ssbo_LODs[lodId].indexCount;
        //rwssbo_InstDrawCmd[cmdId].indexOffset = ssbo_LODs[lodId].indexOffset;
        //ssbo_DrawCmd[cmdId].vertOffset = 0; // Already added to the index buffer
        //
        //ssbo_DrawCmd[cmdId].instCount = rwssbo_InstCounter[lodId].instanceCount;
        //ssbo_DrawCmd[cmdId].insOffset = 0;
    }
}