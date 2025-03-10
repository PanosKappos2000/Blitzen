#pragma once

#include "Core/blitzenContainerLibrary.h"
#include "Engine/blitzenEngine.h"
#include "Renderer/blitRenderingResources.h"

namespace BlitzenEngine
{
    constexpr uint32_t ce_maxObjectCount = 5'000'000;
    

    class GameObject
    {
    public:
        GameObject(const char* meshName = "undefined", uint8_t bDynamic = 0);

		inline uint8_t IsDynamic() const { return m_bDynamic; } 

        uint32_t m_meshId; // Index into the mesh array inside of the LoadedResources struct in BlitzenEngine

        uint32_t m_transformId;

        virtual void Update();

		virtual ~GameObject() = default;

    private: 
		uint8_t m_bDynamic; // If the object is dynamic, it will be updated every frame
    };

    struct GameObjectManager
    {
    private:
        template<class T>
        using Entity = BlitCL::SmartPointer<T, BlitzenCore::AllocationType::Entity>;
        BlitCL::DynamicArray<Entity<GameObject>> m_gameObjects;
        
        BlitCL::DynamicArray<GameObject*> m_pDynamicObjects;

    public:
        // Handles the addition of game objects to the scene
        template<typename T, typename... Args>
        inline uint8_t AddObject(BlitzenEngine::RenderingResources* pResources, 
            const BlitzenEngine::MeshTransform& initialTransform, Args... args)
        {
			if (m_gameObjects.GetSize() >= ce_maxObjectCount)
			{
				BLIT_ERROR("Maximum object count reached")
				return 0;
			}

			m_gameObjects.Resize(m_gameObjects.GetSize() + 1);
            auto& newEntity = m_gameObjects.Back();
            newEntity.MakeAs<T>(args...);

            newEntity->m_transformId = pResources->AddRenderObjectsFromMesh(
                newEntity->m_meshId, initialTransform
            );
			if (m_gameObjects.Back()->IsDynamic())
			{
                m_pDynamicObjects.PushBack(m_gameObjects.Back().Data());
			}

            return 1;
        }

        inline void UpdateDynamicObjects()
        {
            for (auto pObject : m_pDynamicObjects)
                pObject->Update();
        }
    };

    class ClientTest : public GameObject
    {
    public:

        void Update() override;

        ClientTest(const char* meshName = "undefined", uint8_t bDynamic = 0);

    private:
        float m_pitch = 0.f;
        float m_yaw = 0.f;
        float m_roll = 0.f;

        float m_speed = 1.f;
    };

}