#pragma once

#include "Core/blitzenContainerLibrary.h"
#include "Engine/blitzenEngine.h"
#include "Renderer/blitRenderingResources.h"

namespace BlitzenEngine
{
    constexpr uint32_t ce_maxObjectCount = 1'000;
    

    class GameObject
    {
    public:
        GameObject(uint8_t bDynamic = 0, const char* meshName = ce_defaultMeshName);

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
        BlitCL::StaticArray<Entity<GameObject>, ce_maxObjectCount> m_gameObjects;
        uint32_t m_objectCount = 0;
        
        BlitCL::DynamicArray<GameObject*> m_pDynamicObjects;// Objects that will call Update

    public:

        // Handles the addition of game objects to the scene
        template<typename T, typename... Args>
        inline uint8_t AddObject(BlitzenEngine::RenderingResources* pResources, 
            const BlitzenEngine::MeshTransform& initialTransform, Args... args)
        {
			if (m_objectCount >= ce_maxObjectCount)
			{
				BLIT_ERROR("Maximum object count reached")
				return 0;
			}

            auto& entity = m_gameObjects[m_objectCount++];
            entity.MakeAs<T>(args...);// Adds a derived game object to the array
            entity->m_transformId = pResources->AddRenderObjectsFromMesh(
                entity->m_meshId, initialTransform
            );

			if (entity->IsDynamic())
			{
                m_pDynamicObjects.PushBack(entity.Data());
			}

            return 1;
        }

        inline void UpdateDynamicObjects()
        {
            for (auto pObject : m_pDynamicObjects)
                pObject->Update();
        }
    };

    // Temporary functionality tester
    class ClientTest : public GameObject
    {
    public:

        void Update() override;

        ClientTest(uint8_t bDynamic = 0, const char* meshName = ce_defaultMeshName);

    private:
        float m_pitch = 0.f;
        float m_yaw = 0.f;
        float m_roll = 0.f;

        float m_speed = 0.1f;
    };




    /*
        General game object functionality
    */

    // Changes orientation values of an object and updates the renderer
    void RotateObject(float& yaw, float& pitch, uint32_t transformId, float speed);

}