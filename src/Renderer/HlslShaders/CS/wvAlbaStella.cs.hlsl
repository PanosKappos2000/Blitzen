#include "../Headers/dynamicCull.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/wvCollision.hlsl"
#include "../Headers/colliders.hlsl"
#include "../../Resources/blitShaderShared.h"

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint objID = dispatchThreadID.x;
    if (objID == hitterID)
    {
        return;
    }
    uint gridCellID = rwssbo_HostTransform[hitterID].cellID;
    if (objID >= rw_Cells[gridCellID].dynamicColliderCount)
    {
        return;
    }
    
    uint receiverID = rw_ColliderIndices[rw_Cells[gridCellID].dynamicColliderOffset + objID];
    float4 hitterAMaxRad = rwssbo_TransformedColliderAMaxRad[hitterID];
    float4 hitterBMinType = rwssbo_TransformedColliderBMinType[hitterID];
    float4 receiverAMaxRad = rwssbo_TransformedColliderAMaxRad[receiverID];
    float4 receiverBMinType = rwssbo_TransformedColliderBMinType[receiverID];
    
    uint hitterColliderType = (uint) hitterBMinType.w;
    uint receiverColliderType = (uint) receiverBMinType.w;
    
    switch (hitterColliderType)
    {
        case BlitzenColliderTypeCapsule:
            switch (receiverColliderType)
            {
                case BlitzenColliderTypeCapsule:
                    if (CheckCollisionCapsuleToCapsule(hitterAMaxRad, hitterBMinType, receiverAMaxRad, receiverBMinType))
                    {
                        // Create event
                    }
                    break;
                case BlitzenColliderTypeAABB:
                    if (CheckCollisionCapsuleToAABB(hitterAMaxRad, hitterBMinType, receiverAMaxRad, receiverBMinType))
                    {
                        // Create event
                    }
                    break;
                case BlitzenColliderTypeSphere:
                    if (CheckCollisionCapsuleToSphere(hitterAMaxRad, hitterBMinType, receiverAMaxRad))
                    {
                        // Create event
                    }
                    break;
            }
            break;
        
        case BlitzenColliderTypeAABB:
            switch (receiverColliderType)
            {
                case BlitzenColliderTypeCapsule:
                    if(CheckCollisionCapsuleToAABB(receiverAMaxRad, receiverBMinType, hitterAMaxRad, hitterBMinType))
                    {
                        // Create event
                    }
                    break;
                case BlitzenColliderTypeAABB:
                    if(CheckCollisionAABBToAABB(hitterAMaxRad, hitterBMinType, receiverAMaxRad, receiverBMinType))
                    {
                        // Create event
                    }
                    break;
                case BlitzenColliderTypeSphere:
                    if(CheckCollisionAABBToSphere(hitterAMaxRad, hitterBMinType, receiverAMaxRad))
                    {
                        // Create event
                    }
                    break;
            }
            break;
        
        case BlitzenColliderTypeSphere:
            switch (receiverColliderType)
            {
                case BlitzenColliderTypeCapsule:
                    if(CheckCollisionCapsuleToSphere(receiverAMaxRad, receiverBMinType, hitterAMaxRad))
                    {
                        // Create event
                    }
                    break;
                case BlitzenColliderTypeAABB:
                    if (CheckCollisionAABBToSphere(receiverAMaxRad, receiverBMinType, hitterAMaxRad))
                    {
                        // Create event
                    }
                    break;
                case BlitzenColliderTypeSphere:
                    if(CheckCollisionSphereToSphere(hitterAMaxRad, receiverAMaxRad))
                    {
                        // Create event
                    }
                    break;
            }
            break;
    }
}