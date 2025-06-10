#pragma once

#include "BlitCL/blitSmartPointer.h"
#include "BlitCL/blitStack.h"
#include "Renderer/Resources/RenderObject/blitRender.h"
#include "Renderer/Entities/Collision/blitCollision.h"
#include "Renderer/Entities/DynamicTransform/blitDynamicTransform.h"


namespace BlitzenEngine
{
    using ENTITY_CREATION_FLAGS = int64_t;

    constexpr ENTITY_CREATION_FLAGS ENTITY_CREATE_GAME_LOGIC_UPDATE = 100;
    constexpr ENTITY_CREATION_FLAGS ENTITY_CREATE_DYNAMIC_TRANSFORM = 200;

    class EntityManager
    {
    public:

        DynamicTransform* m_pMoving[BlitzenCore::Ce_MaxDynamicObjectCount];
        uint32_t m_movingCount{ 0 };

        BlitCL::BlitStack<uint32_t, BlitzenCore::Ce_MaxDynamicObjectCount> m_availableMovingSpots;

        BlitCL::BlitStack<DynamicTransform*, BlitzenCore::Ce_MaxDynamicObjectCount> m_movingEntities;

        DynamicTransform m_dynamicTransforms[BlitzenCore::Ce_MaxDynamicObjectCount];
        uint32_t m_dynamicTransformCount{ 0 };

        CollisionGrid m_collisionGrids[BlitzenCore::Ce_MaxWorldCollisionGridCount];
        uint32_t m_collisionGridCount{ 0 };

        Collision m_collisions[BlitzenCore::Ce_MaxDynamicObjectCount]{};
        uint32_t m_collisionCount{ 0 };

        BlitzenEngine::RenderContainer m_renderContainer;
    };

    void AddMovingEntitiesToManager(EntityManager* pContext, uint32_t idx);

    void RemoveMovingEntitiesFromManager(EntityManager* pContext, uint32_t idx);
    
    using EntitySystemMemory = BlitCL::SmartPointer<BlitzenEngine::EntityManager, BlitzenCore::AllocationType::Entity>;
}