#pragma once
#include "blitEvents.h"

namespace BlitzenCore
{
    class GameObjectManager
    {
    private:

        template<class T>
        using Entity = BlitCL::SmartPointer<T, BlitzenCore::AllocationType::Entity>;

        BlitCL::StaticArray<Entity<BlitzenEngine::GameObject>, BlitzenCore::Ce_MaxDynamicObjectCount> m_gameObjects;
        uint32_t m_objectCount = 0;

        BlitCL::DynamicArray<BlitzenEngine::GameObject*> m_pDynamicObjects;

    public:

        template<class DERIVED, typename... ARGS>
        inline bool AddObject(BlitzenEngine::RenderingResources* pResources, BlitzenEngine::MeshTransform& initialTransform, bool isDynamic, 
			const char* meshName, ARGS&&... args)
        {
            if (m_objectCount >= BlitzenCore::Ce_MaxDynamicObjectCount)
            {
                BLIT_ERROR("Maximum object count reached");
                return false;
            }

            // Adds a derived game object
            auto& entity = m_gameObjects[m_objectCount++];
            entity.MakeAs<DERIVED>(pResources, isDynamic, std::ref(initialTransform), meshName, std::forward<ARGS>(args)...);

            if (entity->IsDynamic())
            {
                m_pDynamicObjects.PushBack(entity.Data());
            }

            return true;
        }

        void UpdateDynamicObjects(BlitzenWorld::BlitzenWorldContext& blitzenContext, BlitzenWorld::BlitzenPrivateContext& privateContext);
    };

    
    void CreateDynamicObjectRendererTest(BlitzenEngine::RenderingResources* pResources, GameObjectManager* pManager);

    bool CreateSceneFromArguments(int argc, char** argv, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer, GameObjectManager* pManager);
    
}