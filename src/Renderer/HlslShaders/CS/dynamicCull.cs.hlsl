#define DRAW_OCCLUSION_TEMPORAL
#define HI_Z_MAP_OCCLUSION

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/cpuShared.h"

cbuffer ObjCountConstant : register(b1)
{
    uint objCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x + BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET;
    
    // Early return if it's out of bounds
    if (objId >= objCount + BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET)
    {
        return;
    }

    Render obj = ssbo_Renders[objId];
    Surface surface = ssbo_Surfaces[obj.surfaceId];
    Movement movement = ssbo_Movements[obj.transformId];
    
    float3 position = ssbo_Transforms[obj.transformId].position + ssbo_Movements[obj.transformId].velocity;
    
    float4 orientationYaw = NormalizedQuatFromAngleAxis(float3(0.f, -1.f, 0.f), movement.rotation.x);
    float4 orientationPitch = NormalizedQuatFromAngleAxis(float3(1.f, 0.f, 0.f), movement.rotation.y);
    
    float4 orientation =  MulitplyQuat(orientationYaw, orientationPitch);
    
    float scale = ssbo_Transforms[obj.transformId].scale;

    // Bounding sphere to view coordinates
    float3 center = RotateQuat(ssbo_BoundingSpheres[objId].center, orientation) * scale + position;
    center = mul(viewMatrix, float4(ssbo_BoundingSpheres[objId].center, 1)).xyz;
    float radius = ssbo_BoundingSpheres[objId].radius * scale;

    // Frustum culling
    if (!FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar))
    {
        return;
    }

    // Occlusion culling
    float4 aabb = float4(0, 0, 0, 0);
    if (ProjectSphere(center, radius, zNear, proj0, proj5, aabb))
    {
        if (!OcclusionCheck(aabb, tex_HiZMap, pyramidWidth, pyramidHeight, center, radius, zNear))
        {
            return;
        }
    }
    
    ssbo_Transforms[obj.transformId].orientation = orientation;
    ssbo_Transforms[obj.transformId].position = position;

    // If the render object gets past culling, lod selection is done and draw command is added
    uint lodId = LODSelection(center, radius, scale, lodTarget, surface.lodOffset, surface.lodCount);
    PrepareDrawCmd(lodId, objId);
}