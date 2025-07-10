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
    Movement movement = rwssbo_HostTransform[objId];
    
    // Starting position at previous
    float3 position = ssbo_Transforms[objId].position;
    // Scale needed for radius and radius needed early for gravity
    float scale = ssbo_Transforms[objId].scale;
    float radius = ssbo_BoundingSpheres[objId].radius * scale;
    
    // If the world variable has movement, start updates
    if (movement.movementFlags != BLIT_RESIDENT_MOVEMENT_NONE)
    {
        // Starts with identity quaternion
        float4 orientation = float4(0.f, 0.f, 0.f, 1.f);
        
        // Checks yaw rotation
        if (movement.movementFlags & BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT)
        {
            float4 orientationYaw = NormalizedQuatFromAngleAxis(float3(0.f, 1.f, 0.f), movement.rotation.y);
            orientation = MultiplyQuat(orientation, orientationYaw);
        }
        
        // Checks pitch rotation
        if (movement.movementFlags & BLIT_RESIDENT_MOVEMENT_ROTATING_PITCH_BIT)
        {
            float4 orientationPitch = NormalizedQuatFromAngleAxis(float3(1.f, 0.f, 0.f), movement.rotation.x);
            orientation = MultiplyQuat(orientation, orientationPitch);
        }
        
        // Checks roll orientation (later)
        if (movement.movementFlags & BLIT_RESIDENT_MOVEMENT_ROTATING_ROLL_BIT)
        {
            //float4 orientationRoll = NormalizedQuatFromAngleAxis(float3(0.f, 0.f, 1.f), movement.rotation.z);
            //orientation = MultiplyQuat(orientation, orientationRoll);
        }
        
        // Updates orientation (the logic here is wrong TODO: FIX)
        ssbo_Transforms[obj.transformId].orientation = orientation;
        
        // Updates position in case something on the CPU side
        position = rwssbo_HostTransform[objId].velocity;
        
        // Check for gravity
        if ((movement.movementFlags & BLIT_RESIDENT_MOVEMENT_GRAVITY_BIT))
        {
            // Height map logic
            if (position.x >= 0 && position.x < BLIT_TERRAIN_GRID_SIZE_TEMP && position.z >= 0 && position.z < BLIT_TERRAIN_GRID_SIZE_TEMP)
            {
                int gridX = (int) floor(position.x);
                int gridZ = (int) floor(position.z);
                int heightDataIndex = gridX + gridZ * BLIT_TERRAIN_GRID_SIZE_TEMP;
                float heightBelow = ssbo_TerrainHeight[heightDataIndex] + radius;
                if(position.y > heightBelow)
                {
                    position.y = max(position.y - BLIT_GRAVITATIONAL_ACCELERATION, heightBelow);
                    rwssbo_HostTransform[obj.transformId].velocity.y = position.y;
                }
                else if(position.y < heightBelow)
                {
                    position.y = min(position.y + 0.02f, heightBelow);
                    rwssbo_HostTransform[obj.transformId].velocity.y = position.y;
                }
            }
            else
            {
                if (position.y > BLIT_TERRAIN_HEIGHT_TEST_VALUE)
                {
                    position.y = max(position.y - BLIT_GRAVITATIONAL_ACCELERATION, BLIT_TERRAIN_HEIGHT_TEST_VALUE);
                    rwssbo_HostTransform[obj.transformId].velocity.y = position.y;
                }
                else if (position.y < BLIT_TERRAIN_HEIGHT_TEST_VALUE)
                {
                    position.y = BLIT_TERRAIN_HEIGHT_TEST_VALUE;
                    rwssbo_HostTransform[obj.transformId].velocity.y = position.y;
                }
            }
        }
    }
    
    float4 newOrientation = ssbo_Transforms[obj.transformId].orientation;
    ssbo_Transforms[obj.transformId].position = position;

    // Bounding sphere to view coordinates
    float3 center = RotateQuat(ssbo_BoundingSpheres[objId].center, newOrientation) * scale + position;
    center = mul(viewMatrix, float4(center, 1)).xyz;

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