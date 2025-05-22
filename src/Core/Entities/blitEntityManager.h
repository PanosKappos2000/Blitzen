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

        BlitCL::StaticArray<Entity<BlitzenEngine::GameObject>, BlitzenCore::Ce_MaxDynamicObjectCount> m_gameObjects;
        uint32_t m_objectCount = 0;

        BlitCL::DynamicArray<BlitzenEngine::GameObject*> m_pDynamicObjects;

    public:

		inline BlitzenEngine::RenderContainer& GetRenderContainer()
		{
			return m_renderContainer;
		}

        // TRANSFORMS
        inline BlitzenEngine::MeshTransform& GetTransform(uint32_t id) const
        {
            return m_renderContainer.m_transforms[id];
        }
		inline BlitzenEngine::MeshTransform* GetTransforms()
		{
			return m_renderContainer.m_transforms.Data();
		}
		inline uint32_t GetTransformCount() const
		{
			return (uint32_t)m_renderContainer.m_transforms.GetSize();
		}
        inline uint32_t GetDynamicTransformCount() const
        {
			return m_renderContainer.m_dynamicTransformCount;
        }

        // RENDERS
        inline BlitzenEngine::RenderObject* GetRenders()
		{
			return m_renderContainer.m_renders;
		}
        inline uint32_t GetRenderCount() const
        {
            return m_renderContainer.m_renderCount;
        }

        inline BlitzenEngine::RenderObject* GetTransparentRenders()
        {
			return m_renderContainer.m_transparentRenders.Data();
        }
		inline uint32_t GetTransparentRenderCount() const
		{
			return (uint32_t)m_renderContainer.m_transparentRenders.GetSize();
		}

		inline BlitzenEngine::RenderObject* GetOnpcRenders()
		{
			return m_renderContainer.m_onpcRenders;
		}
        inline uint32_t GetOnpcRenderCount() const
        {
			return m_renderContainer.m_onpcRenderCount;
        }

        template<class DERIVED, typename... ARGS>
        inline bool AddObject(BlitzenEngine::RenderContainer& renders, BlitzenEngine::MeshResources& meshes, BlitzenEngine::MeshTransform& initialTransform, bool isDynamic, 
			const char* meshName, ARGS&&... args)
        {
            if (m_objectCount >= BlitzenCore::Ce_MaxDynamicObjectCount)
            {
                BLIT_ERROR("Maximum object count reached");
                return false;
            }

            // Adds a derived game object
            auto& entity = m_gameObjects[m_objectCount++];
            entity.MakeAs<DERIVED>(renders, meshes, isDynamic, std::ref(initialTransform), meshName, std::forward<ARGS>(args)...);

            if (entity->IsDynamic())
            {
                m_pDynamicObjects.PushBack(entity.Data());
            }

            return true;
        }

    private:

        BlitzenEngine::RenderContainer m_renderContainer;
    };
    
}