#pragma once
#include "Core/Events/blitTimeManager.h"
#include "Renderer/Interface/blitRenderer.h"
#include "BlitCL/blitPfn.h"
#include "Renderer/Entities/Residents/blitResidentManager.h"

namespace BlitzenWorld
{
    enum class WorldUpdateState : uint8_t
    {
        PREPARING = 0,
        READY = 1,
        DISPATCHED = 2,
        WAITING = 3,
    };

    enum class WorldUpdateType : int32_t
    {
        NO_UPDATE = 0,

        ADD_DYNAMIC_TRANSFORM = 1,
        REMOVE_DYNAMIC_TRANSFORM = 2,
        CHANGE_DYNAMIC_TRANSFORM = 3,
        BLOCK_DYNAMIC_TRANSFORM = 4
    };
    constexpr uint32_t CE_WORLD_UPDATE_RECEIVER_MAX_WORK_COUNT = 10;

    struct BlitzenWorldUpdate
    {
        WorldUpdateType m_worldUpdateFlags[CE_WORLD_UPDATE_RECEIVER_MAX_WORK_COUNT]{ WorldUpdateType::NO_UPDATE };
        uint32_t m_updateIDX[CE_WORLD_UPDATE_RECEIVER_MAX_WORK_COUNT]{ 0 };
        uint8_t m_worldUpdateWorkCount{ 0 };
        WorldUpdateState m_state{ WorldUpdateState::WAITING };
    };

    // WORLD variable. Represent the idea of world interaction
    struct WORLD_blit
    {
        // Scene resources
        BlitCL::DynamicArray<BlitzenEngine::SceneContext> m_scenes{};

        // Camera should be component. Does not belong here. The container is also a dumb idea. The need for an extra camera should just give rise to a new system
        BlitzenEngine::CameraContainer* pCameraContainer{ nullptr };

        // This will be moved to a renderer manager.
        BlitzenEngine::Renderer P_RENDERER;
        BlitzenEngine::DrawContext m_drawContext;

        // World residents
        BlitzenEngine::WORLD_RESIDENTS m_residents;

        float deltaTime;

        inline WORLD_blit(BlitzenEngine::Camera& camera, BlitzenEngine::MeshResources& meshes, BlitzenEngine::RenderContainer& renders, 
            BlitzenEngine::TextureManager& textureManager, BlitzenPlatform::PlatformContext* pPlatform)
            :m_drawContext{ camera, meshes, renders, textureManager, pPlatform }
        {

        }
    };
}