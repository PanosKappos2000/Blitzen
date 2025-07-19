#include "blitColliders.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "BlitzenMathLibrary/blitML.h"
#include "BlitCL/blitDynamicArr.h"
#include "BlitzenMathLibrary/blitMLSIMD.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"

namespace BlitzenEngine
{
#if defined(BLIT_SIMD_COLLISION_RESOLVE)
    constexpr bool GCSIMDCollisionResolveFlag = true;
#else
    constexpr bool GCSIMDCollisionResolveFlag = false;
#endif
    void ColliderContainer::CheckCapsuleColliderInsideGridCell(Resident hitter, GridCellOffsets& cell, Resident* indices)
    {
        auto& hitterAMaxRad = MTransformedColliderAMaxRad[hitter].data;
        auto& hitterBMinType = MTransformedColliderBMinType[hitter].data;

        if constexpr (GCBlitzenSimd && GCSIMDCollisionResolveFlag)
        {
            Resident* staticColliderIndicesArr = &indices[cell.staticCollidersOffset];

            uint32_t id = 0;
            while ( id < cell.staticColliderCount)
            {
                Resident firstReceiver = staticColliderIndicesArr[id];
                BlitzenColliderType previousColliderType = (BlitzenColliderType)MColliderBMinType[firstReceiver].data.w;
                BlitML::float4 receiverAMaxRadArr[GCSIMDCollisionDataMaxElementCount];
                BlitML::float4 receiverBMinTypeArr[GCSIMDCollisionDataMaxElementCount];
                BlitzenCore::FAT_BOOL collisionBatchResults[GCSIMDCollisionDataMaxElementCount];
                Resident receiverArr[GCSIMDCollisionDataMaxElementCount];
                uint32_t batchCount = 0;

                // Batches mutliple colliders of the same type in the same array for optimal SIMD
                // Maximum is GCSIMDCollisionDataMaxElementCount
                while (id < cell.staticColliderCount && batchCount < GCSIMDCollisionDataMaxElementCount)
                {
                    Resident currentReceiver = staticColliderIndicesArr[id];
                    BlitML::float4& currentReceiverAMaxRad = MColliderAMaxRad[currentReceiver].data;
                    BlitML::float4& currentReceiverBMinType = MColliderBMinType[currentReceiver].data;
                    BlitzenColliderType colliderType = (BlitzenColliderType)currentReceiverBMinType.w;

                    if (colliderType == previousColliderType)
                    {
                        receiverArr[batchCount] = currentReceiver;
                        receiverAMaxRadArr[batchCount] = currentReceiverAMaxRad;
                        receiverBMinTypeArr[batchCount] = currentReceiverBMinType;
                        batchCount++;
                        id++;
                        previousColliderType = colliderType;
                    }
                    else
                    {
                        break;
                    }
                }

                // Dispatches actual check on the batch
                switch (previousColliderType)
                {
                case BlitzenColliderTypeCapsule:
                    BCPSS::CheckCapsuleCollisionOnMultipleCapsules(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeAABB:
                    BCPSS::CheckCapsuleCollisionOnMultipleAABBs(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeSphere:
                    BCPSS::CheckCapsuleCollisionOnMultipleSpheres(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, batchCount, collisionBatchResults);
                    break;
                }

                for (uint32_t res = 0; res < batchCount; res++)
                {
                    if (collisionBatchResults[res] == BLIT_FAT_TRUE) MCollsionMessage[mCollisionMessageCount++] = { hitter, receiverArr[res] };
                }
            }

            Resident* dynamicColliderIndicesArr = &indices[cell.dynamicCollidersOffset];

            id = 0;
            while (id < cell.dynamicCollidersCount)
            {
                Resident firstReceiver = dynamicColliderIndicesArr[id];
                BlitzenColliderType previousColliderType = (BlitzenColliderType)MColliderBMinType[firstReceiver].data.w;
                BlitML::float4 receiverAMaxRadArr[GCSIMDCollisionDataMaxElementCount];
                BlitML::float4 receiverBMinTypeArr[GCSIMDCollisionDataMaxElementCount];
                BlitzenCore::FAT_BOOL collisionBatchResults[GCSIMDCollisionDataMaxElementCount];
                Resident receiverArr[GCSIMDCollisionDataMaxElementCount];
                uint32_t batchCount = 0;

                // Batches mutliple colliders of the same type in the same array for optimal SIMD
                // Maximum is GCSIMDCollisionDataMaxElementCount
                while (id < cell.dynamicCollidersCount && batchCount < GCSIMDCollisionDataMaxElementCount)
                {
                    Resident currentReceiver = dynamicColliderIndicesArr[id];
                    BlitML::float4& currentReceiverAMaxRad = MTransformedColliderAMaxRad[currentReceiver].data;
                    BlitML::float4& currentReceiverBMinType = MTransformedColliderBMinType[currentReceiver].data;
                    BlitzenColliderType colliderType = (BlitzenColliderType)currentReceiverBMinType.w;

                    if (colliderType == previousColliderType)
                    {
                        receiverArr[batchCount] = currentReceiver;
                        receiverAMaxRadArr[batchCount] = currentReceiverAMaxRad;
                        receiverBMinTypeArr[batchCount] = currentReceiverBMinType;
                        batchCount++;
                        id++;
                        previousColliderType = colliderType;
                    }
                    else
                    {
                        break;
                    }
                }

                // Dispatches actual check on the batch
                switch (previousColliderType)
                {
                case BlitzenColliderTypeCapsule:
                    BCPSS::CheckCapsuleCollisionOnMultipleCapsules(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeAABB:
                    BCPSS::CheckCapsuleCollisionOnMultipleAABBs(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeSphere:
                    BCPSS::CheckCapsuleCollisionOnMultipleSpheres(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, batchCount, collisionBatchResults);
                    break;
                }

                for (uint32_t res = 0; res < batchCount; res++)
                {
                    if (collisionBatchResults[res] == BLIT_FAT_TRUE)
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiverArr[res] };
                        if (!CheckResidentVelocity(receiverArr[res])) MCollsionMessage[mCollisionMessageCount++] = { receiverArr[res], hitter };
                    }
                }
            }
        }
        else
        {
            Resident* staticColliderIndicesArr = &indices[cell.staticCollidersOffset];

            for (uint32_t id = 0; id < cell.staticColliderCount; ++id)
            {
                Resident receiver = staticColliderIndicesArr[id];

                auto& receiverAMaxRad = MColliderAMaxRad[receiver].data;
                auto& receiverBMinType = MColliderBMinType[receiver].data;

                BlitzenColliderType colliderType = (BlitzenColliderType)receiverBMinType.w;
                switch (colliderType)
                {
                case BlitzenColliderTypeCapsule:
                    if (BCPSS::CheckCollisionCapsuleToCapsule(hitterAMaxRad, hitterBMinType, receiverAMaxRad, receiverBMinType))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                    }
                    break;

                case BlitzenColliderTypeAABB:
                    if (BCPSS::CheckCollisionCapsuleToAABB(hitterAMaxRad, hitterBMinType, receiverAMaxRad, receiverBMinType))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                    }
                    break;

                case BlitzenColliderTypeSphere:
                    if (BCPSS::CheckCollisionCapsuleToSphere(hitterAMaxRad, hitterBMinType, receiverAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                    }
                    break;
                default:
                    BLIT_ERROR("%s: Unexpected shape during narrow phase checks", BlitzenCore::GCCollisionSystemName);
                    break;
                }
            }

            Resident* dynamicColliderIndicesArr = &indices[cell.dynamicCollidersOffset];

            for (uint32_t id = 0; id < cell.dynamicCollidersCount; ++id)
            {
                Resident receiver = dynamicColliderIndicesArr[id];
                if (hitter == receiver) continue;

                auto& receiverAMaxRad = MTransformedColliderAMaxRad[receiver].data;
                auto& receiverBMinType = MTransformedColliderBMinType[receiver].data;

                BlitzenColliderType colliderType = (BlitzenColliderType)receiverBMinType.w;
                switch (colliderType)
                {
                case BlitzenColliderTypeCapsule:
                    if (BCPSS::CheckCollisionCapsuleToCapsule(hitterAMaxRad, hitterBMinType, receiverAMaxRad, receiverBMinType))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                        // Adds event both ways, unless the receiver has velocity. 
                        // If the receiver has velocity, they will create it themselves
                        if (!CheckResidentVelocity(receiver)) MCollsionMessage[mCollisionMessageCount++] = { receiver, hitter };
                    }
                    break;

                case BlitzenColliderTypeAABB:
                    if (BCPSS::CheckCollisionCapsuleToAABB(hitterAMaxRad, hitterBMinType, receiverAMaxRad, receiverBMinType))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                        // Adds event both ways, unless the receiver has velocity. 
                        // If the receiver has velocity, they will create it themselves
                        if (!CheckResidentVelocity(receiver)) MCollsionMessage[mCollisionMessageCount++] = { receiver, hitter };
                    }
                    break;

                case BlitzenColliderTypeSphere:
                    if (BCPSS::CheckCollisionCapsuleToSphere(hitterAMaxRad, hitterBMinType, receiverAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                        // Adds event both ways, unless the receiver has velocity. 
                        // If the receiver has velocity, they will create it themselves
                        if (!CheckResidentVelocity(receiver)) MCollsionMessage[mCollisionMessageCount++] = { receiver, hitter };
                    }
                    break;
                default:
                    BLIT_ERROR("%s: Unexpected shape during narrow phase checks", BlitzenCore::GCCollisionSystemName);
                    break;
                }
            }
        }
    }

    void ColliderContainer::CheckAABBColliderInsideGridCell(Resident hitter, GridCellOffsets& cell, Resident* indices)
    {
        BlitML::float4& hitterAMaxRad = MTransformedColliderAMaxRad[hitter].data;
        BlitML::float4& hitterBMinType = MTransformedColliderBMinType[hitter].data;

        if constexpr (GCBlitzenSimd && GCSIMDCollisionResolveFlag)
        {
            Resident* staticColliderIndicesArr = &indices[cell.staticCollidersOffset];

            uint32_t id = 0;
            while (id < cell.staticColliderCount)
            {
                Resident firstReceiver = staticColliderIndicesArr[id];
                BlitzenColliderType previousColliderType = (BlitzenColliderType)MColliderBMinType[firstReceiver].data.w;
                BlitML::float4 receiverAMaxRadArr[GCSIMDCollisionDataMaxElementCount];
                BlitML::float4 receiverBMinTypeArr[GCSIMDCollisionDataMaxElementCount];
                BlitzenCore::FAT_BOOL collisionBatchResults[GCSIMDCollisionDataMaxElementCount];
                Resident receiverArr[GCSIMDCollisionDataMaxElementCount];
                uint32_t batchCount = 0;

                // Batches mutliple colliders of the same type in the same array for optimal SIMD
                // Maximum is GCSIMDCollisionDataMaxElementCount
                while (id < cell.staticColliderCount && batchCount < GCSIMDCollisionDataMaxElementCount)
                {
                    Resident currentReceiver = staticColliderIndicesArr[id];
                    BlitML::float4& currentReceiverAMaxRad = MColliderAMaxRad[currentReceiver].data;
                    BlitML::float4& currentReceiverBMinType = MColliderBMinType[currentReceiver].data;
                    BlitzenColliderType colliderType = (BlitzenColliderType)currentReceiverBMinType.w;

                    if (colliderType == previousColliderType)
                    {
                        receiverArr[batchCount] = currentReceiver;
                        receiverAMaxRadArr[batchCount] = currentReceiverAMaxRad;
                        receiverBMinTypeArr[batchCount] = currentReceiverBMinType;
                        batchCount++;
                        id++;
                        previousColliderType = colliderType;
                    }
                    else
                    {
                        break;
                    }
                }

                // Dispatches actual check on the batch
                switch (previousColliderType)
                {
                case BlitzenColliderTypeCapsule:
                    BCPSS::CheckAABBCollisionOnMultipleCapsules(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeAABB:
                    BCPSS::CheckAABBCollisionOnMultipleAABBs(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeSphere:
                    BCPSS::CheckAABBCollisionOnMultipleSpheres(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, batchCount, collisionBatchResults);
                    break;
                }

                for (uint32_t res = 0; res < batchCount; res++)
                {
                    if (collisionBatchResults[res] == BLIT_FAT_TRUE) MCollsionMessage[mCollisionMessageCount++] = { hitter, receiverArr[res] };
                }
            }

            Resident* dynamicColliderIndicesArr = &indices[cell.dynamicCollidersOffset];

            id = 0;
            while (id < cell.dynamicCollidersCount)
            {
                Resident firstReceiver = dynamicColliderIndicesArr[id];
                BlitzenColliderType previousColliderType = (BlitzenColliderType)MColliderBMinType[firstReceiver].data.w;
                BlitML::float4 receiverAMaxRadArr[GCSIMDCollisionDataMaxElementCount];
                BlitML::float4 receiverBMinTypeArr[GCSIMDCollisionDataMaxElementCount];
                BlitzenCore::FAT_BOOL collisionBatchResults[GCSIMDCollisionDataMaxElementCount];
                Resident receiverArr[GCSIMDCollisionDataMaxElementCount];
                uint32_t batchCount = 0;

                // Batches mutliple colliders of the same type in the same array for optimal SIMD
                // Maximum is GCSIMDCollisionDataMaxElementCount
                while (id < cell.dynamicCollidersCount && batchCount < GCSIMDCollisionDataMaxElementCount)
                {
                    Resident currentReceiver = dynamicColliderIndicesArr[id];
                    BlitML::float4 currentReceiverAMaxRad = MTransformedColliderAMaxRad[currentReceiver].data;
                    BlitML::float4 currentReceiverBMinType = MTransformedColliderBMinType[currentReceiver].data;
                    BlitzenColliderType colliderType = (BlitzenColliderType)currentReceiverBMinType.w;

                    if (colliderType == previousColliderType)
                    {
                        receiverArr[batchCount] = currentReceiver;
                        receiverAMaxRadArr[batchCount] = currentReceiverAMaxRad;
                        receiverBMinTypeArr[batchCount] = currentReceiverBMinType;
                        batchCount++;
                        id++;
                        previousColliderType = colliderType;
                    }
                    else
                    {
                        break;
                    }
                }

                // Dispatches actual check on the batch
                switch (previousColliderType)
                {
                case BlitzenColliderTypeCapsule:
                    BCPSS::CheckAABBCollisionOnMultipleCapsules(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeAABB:
                    BCPSS::CheckAABBCollisionOnMultipleAABBs(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeSphere:
                    BCPSS::CheckAABBCollisionOnMultipleSpheres(hitterAMaxRad, hitterBMinType, receiverAMaxRadArr, batchCount, collisionBatchResults);
                    break;
                }

                for (uint32_t res = 0; res < batchCount; res++)
                {
                    if (collisionBatchResults[res] == BLIT_FAT_TRUE)
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiverArr[res] };
                        if (!CheckResidentVelocity(receiverArr[res])) MCollsionMessage[mCollisionMessageCount++] = { receiverArr[res], hitter };
                    }
                }
            }
        }
        else
        {
            Resident* staticColliderIndicesArr = &indices[cell.staticCollidersOffset];

            for (uint32_t id = 0; id < cell.staticColliderCount; ++id)
            {
                Resident receiver = staticColliderIndicesArr[id];

                auto& receiverAMaxRad = MColliderAMaxRad[receiver].data;
                auto& receiverBMinType = MColliderBMinType[receiver].data;

                BlitzenColliderType colliderType = (BlitzenColliderType)receiverBMinType.w;
                switch (colliderType)
                {
                case BlitzenColliderTypeCapsule:
                    if (BCPSS::CheckCollisionCapsuleToAABB(receiverAMaxRad, receiverBMinType, hitterAMaxRad, hitterBMinType))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                    }
                    break;

                case BlitzenColliderTypeAABB:
                    if (BCPSS::CheckCollisionAABBToAABB(hitterAMaxRad, hitterBMinType, receiverAMaxRad, receiverBMinType))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                    }
                    break;

                case BlitzenColliderTypeSphere:
                    if (BCPSS::CheckCollisionAABBToSphere(hitterAMaxRad, hitterBMinType, receiverAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                    }
                    break;
                default:
                    BLIT_ERROR("%s: Unexpected shape during narrow phase checks", BlitzenCore::GCCollisionSystemName);
                    break;
                }
            }

            Resident* dynamicColliderIndicesArr = &indices[cell.dynamicCollidersOffset];

            for (uint32_t id = 0; id < cell.dynamicCollidersCount; ++id)
            {
                Resident receiver = dynamicColliderIndicesArr[id];
                if (hitter == receiver) continue;

                auto& receiverAMaxRad = MTransformedColliderAMaxRad[receiver].data;
                auto& receiverBMinType = MTransformedColliderBMinType[receiver].data;

                BlitzenColliderType colliderType = (BlitzenColliderType)receiverBMinType.w;
                switch (colliderType)
                {
                case BlitzenColliderTypeCapsule:
                    if (BCPSS::CheckCollisionCapsuleToAABB(receiverAMaxRad, receiverBMinType, hitterAMaxRad, hitterBMinType))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                        // Adds event both ways, unless the receiver has velocity. 
                        // If the receiver has velocity, they will create it themselves
                        if (!CheckResidentVelocity(receiver)) MCollsionMessage[mCollisionMessageCount++] = { receiver, hitter };
                    }
                    break;

                case BlitzenColliderTypeAABB:
                    if (BCPSS::CheckCollisionAABBToAABB(hitterAMaxRad, hitterBMinType, receiverAMaxRad, receiverBMinType))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                        // Adds event both ways, unless the receiver has velocity. 
                        // If the receiver has velocity, they will create it themselves
                        if (!CheckResidentVelocity(receiver)) MCollsionMessage[mCollisionMessageCount++] = { receiver, hitter };
                    }
                    break;

                case BlitzenColliderTypeSphere:
                    if (BCPSS::CheckCollisionAABBToSphere(hitterAMaxRad, hitterBMinType, receiverAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                        // Adds event both ways, unless the receiver has velocity. 
                        // If the receiver has velocity, they will create it themselves
                        if (!CheckResidentVelocity(receiver)) MCollsionMessage[mCollisionMessageCount++] = { receiver, hitter };
                    }
                    break;
                default:
                    BLIT_ERROR("%s: Unexpected shape during narrow phase checks", BlitzenCore::GCCollisionSystemName);
                    break;
                }
            }
        }
    }

    void ColliderContainer::CheckSphereColliderInsideGridCell(Resident hitter, GridCellOffsets& cell, Resident* indices)
    {
        auto& hitterAMaxRad = MColliderAMaxRad[hitter].data;

        if constexpr (GCBlitzenSimd && GCSIMDCollisionResolveFlag)
        {
            Resident* staticColliderIndicesArr = &indices[cell.staticCollidersOffset];

            uint32_t id = 0;
            while (id < cell.staticColliderCount)
            {
                Resident firstReceiver = staticColliderIndicesArr[id];
                BlitzenColliderType previousColliderType = (BlitzenColliderType)MColliderBMinType[firstReceiver].data.w;
                BlitML::float4 receiverAMaxRadArr[GCSIMDCollisionDataMaxElementCount];
                BlitML::float4 receiverBMinTypeArr[GCSIMDCollisionDataMaxElementCount];
                BlitzenCore::FAT_BOOL collisionBatchResults[GCSIMDCollisionDataMaxElementCount];
                Resident receiverArr[GCSIMDCollisionDataMaxElementCount];
                uint32_t batchCount = 0;

                // Batches mutliple colliders of the same type in the same array for optimal SIMD
                // Maximum is GCSIMDCollisionDataMaxElementCount
                while (id < cell.staticColliderCount && batchCount < GCSIMDCollisionDataMaxElementCount)
                {
                    Resident currentReceiver = staticColliderIndicesArr[id];
                    BlitML::float4& currentReceiverAMaxRad = MColliderAMaxRad[currentReceiver].data;
                    BlitML::float4& currentReceiverBMinType = MColliderBMinType[currentReceiver].data;
                    BlitzenColliderType colliderType = (BlitzenColliderType)currentReceiverBMinType.w;

                    if (colliderType == previousColliderType)
                    {
                        receiverArr[batchCount] = currentReceiver;
                        receiverAMaxRadArr[batchCount] = currentReceiverAMaxRad;
                        receiverBMinTypeArr[batchCount] = currentReceiverBMinType;
                        batchCount++;
                        id++;
                        previousColliderType = colliderType;
                    }
                    else
                    {
                        break;
                    }
                }

                // Dispatches actual check on the batch
                switch (previousColliderType)
                {
                case BlitzenColliderTypeCapsule:
                    BCPSS::CheckSphereCollisionOnMutlipleCapsules(hitterAMaxRad, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeAABB:
                    BCPSS::CheckSphereCollisionOnMultipleAABBs(hitterAMaxRad, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeSphere:
                    BCPSS::CheckSphereCollisionOnMultipleSpheres(hitterAMaxRad, receiverAMaxRadArr, batchCount, collisionBatchResults);
                    break;
                }

                for (uint32_t res = 0; res < batchCount; res++)
                {
                    if (collisionBatchResults[res] == BLIT_FAT_TRUE) MCollsionMessage[mCollisionMessageCount++] = { hitter, receiverArr[res] };
                }
            }

            Resident* dynamicColliderIndicesArr = &indices[cell.dynamicCollidersOffset];

            id = 0;
            while (id < cell.dynamicCollidersCount)
            {
                Resident firstReceiver = dynamicColliderIndicesArr[id];
                BlitzenColliderType previousColliderType = (BlitzenColliderType)MColliderBMinType[firstReceiver].data.w;
                BlitML::float4 receiverAMaxRadArr[GCSIMDCollisionDataMaxElementCount];
                BlitML::float4 receiverBMinTypeArr[GCSIMDCollisionDataMaxElementCount];
                BlitzenCore::FAT_BOOL collisionBatchResults[GCSIMDCollisionDataMaxElementCount];
                Resident receiverArr[GCSIMDCollisionDataMaxElementCount];
                uint32_t batchCount = 0;

                // Batches mutliple colliders of the same type in the same array for optimal SIMD
                // Maximum is GCSIMDCollisionDataMaxElementCount
                while (id < cell.dynamicCollidersCount && batchCount < GCSIMDCollisionDataMaxElementCount)
                {
                    Resident currentReceiver = dynamicColliderIndicesArr[id];
                    BlitML::float4& currentReceiverAMaxRad = MTransformedColliderAMaxRad[currentReceiver].data;
                    BlitML::float4& currentReceiverBMinType = MTransformedColliderBMinType[currentReceiver].data;
                    BlitzenColliderType colliderType = (BlitzenColliderType)currentReceiverBMinType.w;

                    if (colliderType == previousColliderType)
                    {
                        receiverArr[batchCount] = currentReceiver;
                        receiverAMaxRadArr[batchCount] = currentReceiverAMaxRad;
                        receiverBMinTypeArr[batchCount] = currentReceiverBMinType;
                        batchCount++;
                        id++;
                        previousColliderType = colliderType;
                    }
                    else
                    {
                        break;
                    }
                }

                // Dispatches actual check on the batch
                switch (previousColliderType)
                {
                case BlitzenColliderTypeCapsule:
                    BCPSS::CheckSphereCollisionOnMutlipleCapsules(hitterAMaxRad, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeAABB:
                    BCPSS::CheckSphereCollisionOnMultipleAABBs(hitterAMaxRad, receiverAMaxRadArr, receiverBMinTypeArr, batchCount, collisionBatchResults);
                    break;

                case BlitzenColliderTypeSphere:
                    BCPSS::CheckSphereCollisionOnMultipleSpheres(hitterAMaxRad, receiverAMaxRadArr, batchCount, collisionBatchResults);
                    break;
                }

                for (uint32_t res = 0; res < batchCount; res++)
                {
                    if (collisionBatchResults[res] == BLIT_FAT_TRUE)
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiverArr[res] };
                        if (!CheckResidentVelocity(receiverArr[res])) MCollsionMessage[mCollisionMessageCount++] = { receiverArr[res], hitter };
                    }
                }
            }
        }
        else
        {

            Resident* staticColliderIndicesArr = &indices[cell.staticCollidersOffset];

            for (uint32_t id = 0; id < cell.staticColliderCount; ++id)
            {
                Resident receiver = staticColliderIndicesArr[id];

                auto& receiverAMaxRad = MColliderAMaxRad[receiver].data;
                auto& receiverBMinType = MColliderBMinType[receiver].data;

                BlitzenColliderType colliderType = (BlitzenColliderType)receiverBMinType.w;
                switch (colliderType)
                {
                case BlitzenColliderTypeCapsule:
                    if (BCPSS::CheckCollisionCapsuleToSphere(receiverAMaxRad, receiverBMinType, hitterAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                    }
                    break;

                case BlitzenColliderTypeAABB:
                    if (BCPSS::CheckCollisionAABBToSphere(receiverAMaxRad, receiverBMinType, hitterAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                    }
                    break;

                case BlitzenColliderTypeSphere:
                    if (BCPSS::CheckCollisionSphereToSphere(hitterAMaxRad, receiverAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                    }
                    break;
                default:
                    BLIT_ERROR("%s: Unexpected shape during narrow phase checks", BlitzenCore::GCCollisionSystemName);
                    break;
                }
            }

            Resident* dynamicColliderIndicesArr = &indices[cell.dynamicCollidersOffset];

            for (uint32_t id = 0; id < cell.dynamicCollidersCount; ++id)
            {
                Resident receiver = dynamicColliderIndicesArr[id];
                if (hitter == receiver) continue;

                auto& receiverAMaxRad = MTransformedColliderAMaxRad[receiver].data;
                auto& receiverBMinType = MTransformedColliderBMinType[receiver].data;

                BlitzenColliderType colliderType = (BlitzenColliderType)receiverBMinType.w;
                switch (colliderType)
                {
                case BlitzenColliderTypeCapsule:
                    if (BCPSS::CheckCollisionCapsuleToSphere(receiverAMaxRad, receiverBMinType, hitterAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                        // Adds event both ways, unless the receiver has velocity. 
                        // If the receiver has velocity, they will create it themselves
                        if (!CheckResidentVelocity(receiver)) MCollsionMessage[mCollisionMessageCount++] = { receiver, hitter };
                    }
                    break;

                case BlitzenColliderTypeAABB:
                    if (BCPSS::CheckCollisionAABBToSphere(receiverAMaxRad, receiverBMinType, hitterAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                        // Adds event both ways, unless the receiver has velocity. 
                        // If the receiver has velocity, they will create it themselves
                        if (!CheckResidentVelocity(receiver)) MCollsionMessage[mCollisionMessageCount++] = { receiver, hitter };
                    }
                    break;

                case BlitzenColliderTypeSphere:
                    if (BCPSS::CheckCollisionSphereToSphere(hitterAMaxRad, receiverAMaxRad))
                    {
                        MCollsionMessage[mCollisionMessageCount++] = { hitter, receiver };
                        // Adds event both ways, unless the receiver has velocity. 
                        // If the receiver has velocity, they will create it themselves
                        if (!CheckResidentVelocity(receiver)) MCollsionMessage[mCollisionMessageCount++] = { receiver, hitter };
                    }
                    break;
                default:
                    BLIT_ERROR("%s: Unexpected shape during narrow phase checks", BlitzenCore::GCCollisionSystemName);
                    break;
                }
            }
        }
    }

    void CollisionGrid::DefineGrid(uint32_t origin)
    {
        mOrigin = origin;
        m_minBounds =  origin - (GCCollisionGridExtent / 2);
        m_maxBounds =  origin + (GCCollisionGridExtent / 2);
    }

	void CollisionGrid::CreateCells()
	{
        for (uint32_t cellId = 0; cellId < CE_COLLISION_GRID_CELL_COUNT; ++cellId)
        {
            mCellOffsets[cellId].staticCollidersOffset= 0;
            mCellOffsets[cellId].staticColliderCount = 0;
            mCellOffsets[cellId].dynamicCollidersOffset = 0;
            mCellOffsets[cellId].dynamicCollidersCount = 0;
        }
	}

    void CollisionGrid::PlaceStatics(BlitzenEngine::MeshTransform* transformArr, uint32_t count)
    {
        uint32_t outOfBounds = 0;
        uint32_t badLogic = 0;

        // Temporary array that holds the cell index for each object inside the grid after the first pass
        BlitCL::DynamicArray<int32_t> gridCellIndices{ BLIT_MAX_WORLD_RENDERS, -1};
        uint32_t validColliderCount = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            // Gets true index using offset, and accesses position
            uint32_t IDX = i + BLIT_OPAQUE_STATIC_RENDER_OFFSET;
            BlitML::float3 position = transformArr[IDX].pos;

            // Rejects residents outside of the grid bounds. 
            // In the future such residents should not appear or should be rare edge cases that get handled in an appropriate manner.
            // For example the could be moved to a different grid and warn the user that this happened.
            if (position.x > (float)m_maxBounds || position.x < (float)m_minBounds || position.z > (float)m_maxBounds || position.z < (float)m_minBounds)
            {
                outOfBounds++;
                continue;
            }

            // Promotes the object's position to grid coordinates [0, GRID_EXTENT]. 
            // This step is done so that the resulting index is between 0 and cell count.
            // The regular transform might give negative values or values starting from a number larger than 0.
            position -= BlitML::float3(float(m_minBounds));

            // Sanity. The first check should have removed such objects. If not, I am doing something wrong.
            // I might leave it here indefinitely as this is the static function and it should not be called at runtime
            BLIT_ASSERT_MESSAGE(position.x > 0.f && position.z > 0.f && position.x < (float)GCCollisionGridExtent && position.z < (float)GCCollisionGridExtent, 
                "Something went wrong with collision grid calculations");

            // Creates an index for each axis based on resident position.
            // The id should not be above the cell's extent
            uint32_t cellPosX = BlitML::UMin(uint32_t(position.x / GCCollisionCellExtent), GCCollsionFlatCount - 1);
            uint32_t cellPosZ = BlitML::UMin(uint32_t(position.z / GCCollisionCellExtent), GCCollsionFlatCount - 1);

            // The flat index is retrieved by turning 2D to 1D
            // The xAxis is kept as it is and the Z is multiplied by the cell count on each axis (flat count)
            uint32_t cellIndex = cellPosX + cellPosZ * GCCollsionFlatCount;

            // Something wrong with indexing logic
            if (cellIndex >= CE_COLLISION_GRID_CELL_COUNT)
            {
                badLogic++;
                continue;
            }

            // Places index into collision cell
            gridCellIndices[IDX] = cellIndex;
            mCellOffsets[cellIndex].staticColliderCount++;
            validColliderCount++;
        }

        // Second pass saves offset and resets count so that indices can be placed properly after
        uint32_t globalOffset = 0;
        for (auto& cell : mCellOffsets)
        {
            cell.staticCollidersOffset = globalOffset;
            globalOffset += cell.staticColliderCount;
            cell.staticColliderCount = 0;
        }

        // Allocates the collider index array if it has not been allocated already
        if (m_colliderIndices == nullptr)
        {
            AllocStatics(validColliderCount);
        }

        // Goes through static objects again. 
        // This time it uses the temporary array to get the cell index for simplicity.
        // The collider count is set again
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t IDX = i + BLIT_OPAQUE_STATIC_RENDER_OFFSET;
            if (gridCellIndices[IDX] == -1)
            {
                continue;
            }
            auto& cell = mCellOffsets[gridCellIndices[IDX]];

            m_colliderIndices[cell.staticCollidersOffset + cell.staticColliderCount] = IDX;
            cell.staticColliderCount++;
        }

        BLIT_INFO("%s: Ouf of collsion grid bounds object count: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, outOfBounds);
        BLIT_INFO("%s: Bad grid index calculation: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, badLogic);
    }

    void CollisionGrid::PlaceDynamics(BlitzenEngine::WVTransform* transformArr, uint32_t count)
    {
        // Resets all collider counts from previous frame
        for (auto& cellOffsets : mCellOffsets)
        {
            cellOffsets.dynamicCollidersCount = 0;
        }

        // Goes through all World Variables. Since only World Varialbes can move, they are the ones who will make collsion happen
        for (uint32_t IDX = 0; IDX < count; ++IDX)
        {
            auto& transform{ transformArr[IDX] };

            // Not making a check. I should make sure that dynamic transforms that are outside the grid get discarded or changed
            // CPU cannot afford these checks

            // This might change, but for now the grid does not have a position.
            // Instead every object is placed inside grid space by incrementing to grid bounds
            BlitML::float3 positionGridSpace = BlitML::float3(transform.position - (float)m_minBounds);

            // Gets the grid space position, finds its position on the x and z axis.
            // Finally, uses that position to create a flat index to find the correct cell for the resident
            uint32_t cellPosX = BlitML::UMin(uint32_t(positionGridSpace.x) / GCCollisionCellExtent, GCCollsionFlatCount);
            uint32_t cellPosZ = BlitML::UMin(uint32_t(positionGridSpace.z) / GCCollisionCellExtent, GCCollsionFlatCount);
            uint32_t cellIndex = cellPosX + cellPosZ * GCCollsionFlatCount;

            // Saves the cell Index for the final stage
            transform.targetIdx = cellIndex;
            // Saves the collider count for the next stage (offset stage)
            mCellOffsets[cellIndex].dynamicCollidersCount++;
        }
        
        // Keeps global offset and iterate over every cell
        uint32_t globalOffset = 0;
        for (auto& cellOffsets : mCellOffsets)
        {
            // Uses each cell's count to create an offset for the next cell.
            // Saves the previous offset created by the previous cell
            cellOffsets.dynamicCollidersOffset = globalOffset;
            globalOffset += cellOffsets.dynamicCollidersCount;

            // The collider count is reset so that collider indices can be placed in a flat array
            // The offset and the count will be used to access that array
            cellOffsets.dynamicCollidersCount = 0;
        }

        // This should always be true. The literal reason these steps are taken is that we do not overshoot and broad phase is precise in terms of count
        BLIT_RUNTIME_TEST_CHECK_ASSERT(globalOffset == BLIT_MAX_WORLD_VARIABLE_COUNT);

        // Goes over world variables one last time
        for (uint32_t IDX = 0; IDX < count; ++IDX)
        {
            // Retrieves the old cell index and accesses the resident's cell
            uint32_t cellIndex = transformArr[IDX].targetIdx;
            auto& cell = mCellOffsets[cellIndex];

            // Using collider offset and collider count, the resident's index is written to the flat array.
            WVColliderIndices[cell.dynamicCollidersOffset + cell.dynamicCollidersCount] = IDX;
            // Collider count gets incremented back to what was calculated earlier.
            cell.dynamicCollidersCount++;
        }
    }

    void ColliderContainer::TransformCollidersWithoutBMPR(Resident* movingResidentArr, uint32_t movingResidentCount, WVTransform* transformArr, MeshTransform* gpuTransformArr)
    {
        for (uint32_t id = 0; id < movingResidentCount; ++id)
        {
            Resident resident = movingResidentArr[id];
            auto& transform = transformArr[resident];
            float scale = gpuTransformArr[resident].scale;
            auto& aMaxRad = MColliderAMaxRad[resident];
            auto& bMinType = MColliderBMinType[resident];
            auto& transformedAMaxRad = MTransformedColliderAMaxRad[resident];
            auto& transformedBMinType = MTransformedColliderBMinType[resident];

            //---------------------------------------------------------------------------------------------------------
            // NOTE: This is hard without using BMPR with the current design, because BMPR owns dynamic quaternions
            // - For now colliders will only receive movement and scale
            // - This means that only moving residents will be checked and updated
            // - The BMPR version will probably be more advanced
            //---------------------------------------------------------------------------------------------------------
            uint32_t type = uint32_t(bMinType.data.w);
            switch (type)
            {
            case BlitzenColliderTypeSphere:
                transformedAMaxRad.data.WriteXYZ(aMaxRad.data.xyz() * scale + transform.position);
                transformedAMaxRad.data.w = aMaxRad.data.w * scale;
                break;

            case BlitzenColliderTypeAABB:
                transformedAMaxRad.data.WriteXYZ(aMaxRad.data.xyz() * scale + transform.position);
                transformedBMinType.data.WriteXYZ(bMinType.data.xyz() * scale + transform.position);
                break;

            case BlitzenColliderTypeCapsule:
                transformedAMaxRad.data.WriteXYZ(aMaxRad.data.xyz() * scale + transform.position);
                transformedBMinType.data.WriteXYZ(bMinType.data.xyz() * scale + transform.position);
                transformedAMaxRad.data.w = aMaxRad.data.w * scale;

            }
        }
    }

    void ColliderContainer::AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform, uint32_t renderObjectID, RENDER_OBJECT_TYPE type)
    {
        auto& newcomer{ m_boundingSpheres[renderObjectID] };

        BlitzenCore::BlitMemCopy(&newcomer, pSphere, sizeof(BoundingSphere));

		if (type == RENDER_OBJECT_TYPE::OPAQUE_STATIC)
		{
			newcomer.m_center = BlitML::RotateQuat(newcomer.m_center, transform.orientation) * transform.scale + transform.pos;
			newcomer.m_radius *= transform.scale;
		}
    }

    bool ColliderContainer::LogResidentForCollision(Resident resident, SplitColliderDataPair& data, MeshTransform& residentTransform, WVColliderResponse behavior)
    {
        if (mStaticColliderCount >= CE_MAX_WORLD_COLLIDER_COUNT)
        {
            BLIT_ERROR("%s: Max Collider count exceeded", BlitzenCore::GCCollisionSystemName)
            return false;
        }

        auto& colliderAMaxRad = MColliderAMaxRad[resident];
        auto& colliderBMinType = MColliderBMinType[resident];
        auto& transformedAMaxRad = MColliderAMaxRad[resident];
        auto& transformedBMinType = MColliderBMinType[resident];

        // Write AMaxRad
        colliderAMaxRad.data.WriteXYZ(data.AMaxRad.data.xyz());
        colliderAMaxRad.data.w = data.AMaxRad.data.w;

        // Write BMinType. Type does not change so it is written to the transformed version as well
        colliderBMinType.data.WriteXYZ(data.BMinType.data.xyz());
        colliderBMinType.data.w = data.BMinType.data.w;
        transformedBMinType.data.w = colliderBMinType.data.w;

        uint32_t type = uint32_t(colliderBMinType.data.w);

        // Bake the transform to the collider for static objects
        if (IS_RESIDENT_STATIC(resident))
        { 
            if (colliderBMinType.data.w == BlitzenColliderTypeSphere)
            {
                colliderAMaxRad.data.WriteXYZ(BlitML::RotateQuat(colliderAMaxRad.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
                colliderAMaxRad.data.w *= residentTransform.scale;
            }
            else if (colliderBMinType.data.w == BlitzenColliderTypeAABB)
            {
                colliderAMaxRad.data.WriteXYZ(BlitML::RotateQuat(colliderAMaxRad.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
                colliderBMinType.data.WriteXYZ(BlitML::RotateQuat(colliderBMinType.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
            }
            else if (colliderBMinType.data.w == BlitzenColliderTypeCapsule)
            {
                colliderAMaxRad.data.WriteXYZ(BlitML::RotateQuat(colliderAMaxRad.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
                colliderBMinType.data.WriteXYZ(BlitML::RotateQuat(colliderBMinType.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
                colliderAMaxRad.data.w *= residentTransform.scale;
            }
            else
            {
                BLIT_ASSERT_MESSAGE(false, "Unrecognized collider enumeration");
            }
            mStaticColliderCount++;
        }
        // For dynamics write transforms to transformed array, keep the other ones in the original state
        else
        {
            if (colliderBMinType.data.w == BlitzenColliderTypeSphere)
            {
                transformedAMaxRad.data.WriteXYZ(BlitML::RotateQuat(colliderAMaxRad.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
                transformedAMaxRad.data.w = colliderAMaxRad.data.w * residentTransform.scale;
            }
            else if (colliderBMinType.data.w == BlitzenColliderTypeAABB)
            {
                transformedAMaxRad.data.WriteXYZ(BlitML::RotateQuat(colliderAMaxRad.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
                transformedBMinType.data.WriteXYZ(BlitML::RotateQuat(colliderBMinType.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
            }
            else if (colliderBMinType.data.w == BlitzenColliderTypeCapsule)
            {
                transformedAMaxRad.data.WriteXYZ(BlitML::RotateQuat(colliderAMaxRad.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
                transformedBMinType.data.WriteXYZ(BlitML::RotateQuat(colliderBMinType.data.xyz(), residentTransform.orientation) * residentTransform.scale + residentTransform.pos);
                transformedAMaxRad.data.w = colliderAMaxRad.data.w * residentTransform.scale;
            }
            WVColliderHitData[resident] = behavior;
            mWorldVariableColliderCount++;
        }

        return true;
    }

    void CollisionGrid::FindCollisionsNarrow(BoundingSphere* boundsArr)
    {
        
    }

    void CollisionGrid::BLITZEN_RESOLVE_RESIDENT_COLLISION_EVENTS(WVColliderResponse* colliderArr)
    {
        
    }

    void CollisionGrid::AllocDynamicIndices()
    {
        WVColliderIndices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Entity, BLIT_MAX_WORLD_VARIABLE_COUNT * sizeof(uint32_t)));
    }

    void CollisionGrid::AllocStatics(uint32_t count)
    {
        BLIT_ASSERT(m_colliderIndices == nullptr);

        m_colliderIndices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Entity, count * sizeof(uint32_t)));
        m_colliderIndicesTotal += count;
    }

    CollisionGrid::~CollisionGrid()
    {
        if (m_colliderIndices)
        {
            BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Entity, m_colliderIndices, m_colliderIndicesTotal * sizeof(uint32_t));
        }

        if (WVColliderIndices)
        {
            BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Entity, WVColliderIndices, BLIT_MAX_WORLD_VARIABLE_COUNT * sizeof(uint32_t));
        }
    }
}