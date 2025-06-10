#include "blitEntityInterface.h"

namespace BlitzenEngine
{
    

    ENTITY_CREATE_RES AddEntityToWorld(EntityManager* pManager, MeshResources& meshes, const char* meshName, ENTITY_CREATION_FLAGS ecf, BLIT_ENTITY_CREATE_CONTEXT& context)
    {
        if (pManager->m_entityCount >= BlitzenCore::Ce_MaxDynamicObjectCount)
        {
            BLIT_ERROR("Maximum entity count reached");
            return ENTITY_CREATE_RES::ENTITIES_FULL;
        }


        auto pMesh = &meshes.m_meshMap[meshName];
        uint32_t transformId{ BlitzenEngine::CreateRenderObjectFromMesh(pManager->m_renderContainer, meshes, pMesh->meshId, context.initialTransform, ecf & ENTITY_CREATE_GAME_LOGIC_UPDATE) };

        if (transformId == BlitzenCore::Ce_MaxRenderObjects)
        {
            BLIT_ERROR("Maximum entity count reached");
            return ENTITY_CREATE_RES::ENTITIES_FULL;
        }

        auto& entity = pManager->m_entities[pManager->m_entityCount++];
        entity.m_pMesh = pMesh;
        entity.m_pTransform = &pManager->m_renderContainer.m_transforms[transformId];
        entity.m_transformId = transformId;

        if (ecf & ENTITY_CREATE_GAME_LOGIC_UPDATE)
        {
            if (pManager->m_dynamicCount >= BlitzenCore::Ce_MaxDynamicObjectCount)
            {
                return ENTITY_CREATE_RES::DYNAMICALLY_UPDATED_ENTITIES_FULL;
            }

            auto& dynamic = pManager->m_dynamicEntities[pManager->m_dynamicCount++];

            dynamic.m_pfnUpdate = context.m_updatePfn;
            dynamic.pEntity = &entity;
        }

        if (ecf & ENTITY_CREATE_DYNAMIC_ORIENTATION)
        {
            if (pManager->m_dynamicRotationCount >= BlitzenCore::Ce_MaxDynamicObjectCount)
            {
                return ENTITY_CREATE_RES::DYNAMIC_ORIENTATION_FULL;
            }

            entity.m_pDynamicRotation = &pManager->m_dynamicRotations[pManager->m_dynamicRotationCount++];
            entity.m_pDynamicRotation->m_speed = context.m_rotationSpeed;
        }

        return ENTITY_CREATE_RES::SUCCESS;
    }

    void AddMovingEntitiesToManager(EntityManager* pManager, uint32_t idx)
    {
        BLIT_ASSERT_MESSAGE(!pManager->m_movingEntities.IsFull(), "Exceeded moving entity count");

        auto pNewcomer{ &pManager->m_dynamicTransforms[idx] };

        // NOTE: I could just make sure that available spots are always full when this is empty
        if (pManager->m_availableMovingSpots.IsEmpty())
        {
            pManager->m_movingEntities.Add(pNewcomer);
        }

        uint32_t idx = pManager->m_availableMovingSpots.Pop();
        pManager->m_movingEntities[idx] = pNewcomer;
    }

    void RemoveMovingEntitiesFromManager(EntityManager* pManager, uint32_t idx)
    {
        BLIT_ASSERT_MESSAGE(pManager->m_movingEntities.Count() > idx, "Tried to remove moving entity out of array bounds");

        pManager->m_movingEntities[idx]->m_isMoving = false;
        pManager->m_availableMovingSpots.Add(idx);
    }
}