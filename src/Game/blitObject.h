#pragma once
#include "Core/blitzenEngine.h"
#include "Core/BlitzenWorld/blitzenWorld.h"

namespace BlitzenEngine
{
#if !defined(LAMBDA_GAME_OBJECT_TEST)

    class GameObject
    {
    public:
        GameObject(BlitzenEngine::MeshTransform* pTransform, uint32_t transformId, BlitzenEngine::Mesh* pMesh, bool isDynamic);

        inline bool IsDynamic() const { return m_bDynamic; }

		inline Mesh* GetMesh() const { return m_pMesh; }

		inline MeshTransform* GetTransform() const { return m_pTransform; }
		inline uint32_t GetTransformId() const { return m_transformId; }

        virtual void Update(BlitzenWorld::BlitzenWorldContext& context);

        virtual ~GameObject() = default; 

    private:

        MeshTransform* m_pTransform;
        uint32_t m_transformId;

        Mesh* m_pMesh;

        bool m_bDynamic; 
    };


    // Temporary functionality tester
    class ClientTest : public GameObject
    {
    public:

        void Update(BlitzenWorld::BlitzenWorldContext& context) override;

        ClientTest(MeshTransform* pTransform, uint32_t transformId, Mesh* pMesh, bool isDynamic);

    private:
        float m_pitch = 0.f;
        float m_yaw = 0.f;
        float m_roll = 0.f;

        float m_speed = 0.1f;
    };

    // Changes orientation values of an object and updates the renderer
    void RotateObject(float& yaw, float& pitch, float speed, MeshTransform* pTransform, BlitzenWorld::BlitzenWorldContext& context);
    
#else
    
    struct GameObject
    {
        GameObject(bool bDynamic = false, const char* meshName = ce_defaultMeshName);

        inline bool IsDynamic() const { return m_bDynamic; }

        uint32_t m_meshId;

        uint32_t m_transformId;

        bool m_bDynamic;

        BlitCL::UnknownPointer m_pData;
    };


    class GameObjectManager
    {
    private:

        template<class T>
        using Entity = BlitCL::SmartPointer<T, BlitzenCore::AllocationType::Entity>;

        BlitCL::DynamicArray<GameObject> m_gameObjects;
        uint32_t m_objectCount = 0;

        BlitCL::DynamicArray<BlitCL::Pfn<void, GameObject*>> m_updateFunctions;

        BlitCL::DynamicArray<GameObject*> m_pDynamicObjects;// Objects that will call Update

    public:

        // Handles the addition of game objects to the scene
        template<class T, typename... Args>
        inline bool AddObject(BlitzenEngine::RenderingResources* pResources,
            const BlitzenEngine::MeshTransform& initialTransform,
            BlitCL::Pfn<void, GameObject*> pfn, bool bDynamic, const char* meshName, Args... args)
        {
            if (m_objectCount >= ce_maxObjectCount)
            {
                BLIT_ERROR("Maximum object count reached")
                    return 0;
            }

            m_gameObjects.PushBack(GameObject{ bDynamic, meshName });
            auto& object = m_gameObjects.Back();
            object.m_pData.Make(args...);
            object.m_transformId = pResources->AddRenderObjectsFromMesh(object.m_meshId, initialTransform);

            if (object.IsDynamic())
            {
                m_pDynamicObjects.PushBack(&object);
                m_updateFunctions.PushBack(pfn);
            }

            return true;
        }

        inline void UpdateDynamicObjects()
        {
            for (size_t i = 0; i < m_pDynamicObjects.GetSize(); ++i)
                m_updateFunctions[i](&m_gameObjects[i]);

        }
    };

    using ClientTest = GameObject;
#endif

    

}