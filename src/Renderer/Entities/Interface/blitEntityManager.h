#pragma once
#include "Core/BlitzenWorld/blitzenWorld.h"
#include "blitEntity.h"
#pragma once

#include "BlitCL/blitSmartPointer.h"
#include "Renderer/Resources/RenderObject/blitRender.h"
#include "Renderer/Entities/Collision/blitCollision.h"

namespace BlitzenEngine
{
    using ENTITY_CREATION_FLAGS = int64_t;

    constexpr ENTITY_CREATION_FLAGS ENTITY_CREATE_GAME_LOGIC_UPDATE = 100;
    constexpr ENTITY_CREATION_FLAGS ENTITY_CREATE_DYNAMIC_ORIENTATION = 200;

    class EntityManager
    {
    public:

        Entity m_entities[BlitzenCore::Ce_MaxDynamicObjectCount];
        uint32_t m_entityCount = 0;

        DynamicUpdateEntity m_dynamicEntities[BlitzenCore::Ce_MaxDynamicObjectCount];
        uint32_t m_dynamicCount{ 0 };

        Entity* m_pDirtyEntities[BlitzenCore::Ce_MaxDynamicObjectCount];
        uint32_t m_dirtyEntityCount{ 0 };

        DynamicRotation m_dynamicRotations[BlitzenCore::Ce_MaxDynamicObjectCount];
        uint32_t m_dynamicRotationCount{ 0 };

        CollisionGrid m_collisionGrids[BlitzenCore::Ce_MaxWorldCollisionGridCount];
        uint32_t m_collisionGridCount{ 0 };

        Collision m_collisions[BlitzenCore::Ce_MaxDynamicObjectCount]{};
        uint32_t m_collisionCount{ 0 };

        BlitzenEngine::RenderContainer m_renderContainer;
    };
    
    using EntitySystemMemory = BlitCL::SmartPointer<BlitzenEngine::EntityManager, BlitzenCore::AllocationType::Entity>;
}