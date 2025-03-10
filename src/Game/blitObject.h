#pragma once

#include "Core/blitzenContainerLibrary.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenEngine
{
    constexpr uint32_t ce_maxObjectCount = 5'000'000;
    

    class GameObject
    {
    public:
        inline GameObject(const char* meshName = "undefined", uint8_t bDynamic = 0) :
            m_meshIndex{0/*pResources->meshMap.Get(meshName).transformIndex*/}, 
            m_bDynamic{ bDynamic }, m_transformIndex{ 0 }
        { }

		inline uint8_t IsDynamic() const { return m_bDynamic; } 

        uint32_t m_meshIndex; // Index into the mesh array inside of the LoadedResources struct in BlitzenEngine

        uint32_t m_transformIndex;

        virtual void Update();

    private: 
		uint8_t m_bDynamic; // If the object is dynamic, it will be updated every frame
    };

    struct GameObjectManager
    {
        BlitCL::StaticArray<GameObject, ce_maxObjectCount> m_gameObjects;
        

        BlitCL::DynamicArray<GameObject*> m_pDynamicObjects;

        // Handles the addition of game objects to the scene
        inline uint8_t AddObject(const char* meshName, uint8_t bDynamic = 0)
        {
			if (m_objectCount >= ce_maxObjectCount)
			{
				BLIT_ERROR("Maximum object count reached")
				return 0;
			}

			m_gameObjects[m_objectCount] = GameObject(meshName, bDynamic);
			if (bDynamic)
			{
                m_pDynamicObjects.PushBack(&m_gameObjects[m_objectCount]);
			}
			m_objectCount++;

            return 1;
        }

    private:
        uint32_t m_objectCount = 0; // Number of object added to m_gameObjects
    };
}