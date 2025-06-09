#include "blitEntityInterface.h"

namespace BlitzenEngine
{
    void UpdateGameLogic(RendererPtrType pRenderer, DynamicUpdateEntity* dynamicEntities, uint32_t dynamicEntityCount, float deltaTime)
    {
        for (uint32_t dynamic = 0; dynamic < dynamicEntityCount; ++dynamic)
        {
            auto& update{ dynamicEntities[dynamic] };

            switch (update.m_pfnUpdate(update.pEntity, deltaTime))
            {
            case ENTITY_UPDATE_RES::UPDATE_RENDERER_TRANSFORM:
            {
                pRenderer->UpdateObjectTransform(update.pEntity->m_transformId, update.pEntity->m_pTransform);
                break;
            }
            case ENTITY_UPDATE_RES::NO_FURTHER_UPDATES: default:
            {
                break;
            }
            }
        }
    }

    void UpdateEntityComponents(RendererPtrType pRenderer, EntityManager* pManager, float deltaTime)
    {
        // Update Game Logic
        UpdateGameLogic(pRenderer, pManager->m_dynamicEntities, pManager->m_dynamicCount, deltaTime);

        // Transform bounding spheres for dynamic objects (compute shader?)

        // Call Previous Frame Collision Events 

        // Repurpose Collision Grid (possibly multithreaded, maybe could be checked every other frame?)

        // Check Collisions (depend on game logic and collision grid, maybe could be checked every other frame?)
    }

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

    void RotateEntity(Entity* pEntity, float deltaTime)
    {
        pEntity->m_pDynamicRotation->m_yaw += pEntity->m_pDynamicRotation->m_speed * deltaTime;
        pEntity->m_pDynamicRotation->m_pitch += pEntity->m_pDynamicRotation->m_speed * deltaTime;

        BlitML::vec3 yAxis(0.f, -1.f, 0.f);
        BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, pEntity->m_pDynamicRotation->m_yaw, 0);

        BlitML::vec3 xAxis(1.f, 0.f, 0.f);
        BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, pEntity->m_pDynamicRotation->m_pitch, 0);

        pEntity->m_pTransform->orientation = yawOrientation + pitchOrientation;
    }
}