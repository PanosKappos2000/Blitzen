#pragma once

#include "Core/blitzenContainerLibrary.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenEngine
{
    constexpr uint32_t ce_maxObjectCount = 5'000'000;
    

    class GameObject
    {
    public:
        GameObject(const char* meshName = "undefined", uint8_t bDynamic = 0);

		inline uint8_t IsDynamic() const { return m_bDynamic; } 

        uint32_t m_meshIndex; // Index into the mesh array inside of the LoadedResources struct in BlitzenEngine

        uint32_t m_transformIndex;

        virtual void Update();

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
        inline uint8_t AddObject(Args... args)
        {
			if (m_gameObjects.GetSize() >= ce_maxObjectCount)
			{
				BLIT_ERROR("Maximum object count reached")
				return 0;
			}

			m_gameObjects.Resize(m_gameObjects.GetSize() + 1);
            m_gameObjects.Back().Make(args...);
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
}