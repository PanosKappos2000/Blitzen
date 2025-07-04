#define OPAQUE_DYNAMIC_CULL

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/dynamicCull.hlsl"
#include "../Headers/occlusionCull.hlsl"
#include "../Headers/cullOut.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../../Resources/blitShaderShared.h"

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
    
    float3 position = ssbo_Transforms[obj.transformId].position;
    
    if (movement.movementFlags != BLIT_RESIDENT_MOVEMENT_NONE)
    {
        float4 orientation = float4(0.f, 0.f, 0.f, 1.f);
        if (movement.movementFlags & BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT)
        {
            float4 orientationYaw = NormalizedQuatFromAngleAxis(float3(0.f, -1.f, 0.f), movement.rotation.x);
            orientation = MultiplyQuat(orientation, orientationYaw);
        }
        if (movement.movementFlags & BLIT_RESIDENT_MOVEMENT_ROTATING_PITCH_BIT)
        {
            float4 orientationPitch = NormalizedQuatFromAngleAxis(float3(1.f, 0.f, 0.f), movement.rotation.y);
            orientation = MultiplyQuat(orientation, orientationPitch);
        }
        if (movement.movementFlags & BLIT_RESIDENT_MOVEMENT_ROTATING_ROLL_BIT)
        {
        
        }
        ssbo_Transforms[obj.transformId].orientation = orientation;
        
        if ((movement.movementFlags & BLIT_RESIDENT_MOVEMENT_GRAVITY_BIT) && position.y > -100.f)
        {
            position.y = position.y - BLIT_GRAVITATIONAL_ACCELERATION >= -100.f ? position.y - BLIT_GRAVITATIONAL_ACCELERATION : -100.f;
        }
        
        if (movement.movementFlags & BLIT_RESIDENT_MOVEMENT_MANUAL_VELOCITY_BIT)
        {
            position += movement.velocity * deltaTime;
        }
    }
    
    float4 newOrientation = ssbo_Transforms[obj.transformId].orientation;
    float scale = ssbo_Transforms[obj.transformId].scale;
    ssbo_Transforms[obj.transformId].position = position;

    // Bounding sphere to view coordinates
    float3 center = RotateQuat(ssbo_BoundingSpheres[objId].center, newOrientation) * scale + position;
    center = mul(viewMatrix, float4(center, 1)).xyz;
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
        if (!OcclusionCheck(aabb, pyramidWidth, pyramidHeight, center, radius, zNear))
        {
            return;
        }
    }

    // If the render object gets past culling, lod selection is done and draw command is added
    uint lodId = LODSelection(center, radius, scale, lodTarget, surface.lodOffset, surface.lodCount);
    PrepareDrawCmd(lodId, objId);
}