#pragma once
#include "Core/BlitzenWorld/blitzenWorld.h"
#include "Game/blitObject.h"
#include "BlitCL/blitSmartPointer.h"
#include "Renderer/Resources/RenderObject/blitRender.h"

namespace BlitzenCore
{
    class EntityManager
    {
    public:

        template<class T>
        using Entity = BlitCL::SmartPointer<T, BlitzenCore::AllocationType::Entity>;

        Entity<BlitzenEngine::GameObject> m_entities[BlitzenCore::Ce_MaxDynamicObjectCount];
        uint32_t m_entityCount = 0;

        BlitzenEngine::GameObject* m_pDynamicEntities[Ce_MaxDynamicObjectCount]{ nullptr };
        uint32_t m_dynamicEntityCount{ 0 };

        BlitzenEngine::RenderContainer m_renderContainer;

        template<class DERIVED, typename... ARGS>
        bool AddObject(BlitzenEngine::MeshResources& meshes, BlitzenEngine::MeshTransform& initialTransform, bool isDynamic, 
			const char* meshName, ARGS&&... args)
        {
            if (m_entityCount >= BlitzenCore::Ce_MaxDynamicObjectCount)
            {
                BLIT_ERROR("Maximum object count reached");
                return false;
            }

            auto pMesh = &meshes.m_meshMap[meshName];
            uint32_t transformId{ BlitzenEngine::CreateRenderObjectFromMesh(m_renderContainer, meshes, pMesh->meshId, initialTransform, isDynamic)};

			if (transformId == Ce_MaxRenderObjects)
			{
				BLIT_ERROR("Failed to create render object");
				return false;
			}

            // Adds a derived game object
            auto& entity = m_entities[m_entityCount++];
            entity.MakeAs<DERIVED>(&m_renderContainer.m_transforms[transformId], transformId, pMesh, isDynamic, std::forward<ARGS>(args)...);

            if (entity->IsDynamic())
            {
                m_pDynamicEntities[m_dynamicEntityCount++] = entity.Data();
            }

            return true;
        }
    };
    
}