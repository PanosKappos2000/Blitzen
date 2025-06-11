#pragma once
#include "Renderer/View/blitCamera.h"
#include "Core/Events/blitTimeManager.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/Entities/Interface/blitEntityManager.h"
#include "Renderer/Resources/Scene/blitScene.h"
#include "BlitCL/blitPfn.h"
#include "Renderer/Entities/Residents/blitWV.h"

namespace BlitzenWorld
{
    using WVTICK_blitpfn = BlitCL::Pfn<void, void*, float>;

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

    struct WORLD_blit
    {
        BlitCL::DynamicArray<BlitzenEngine::SceneContext> m_scenes{};
        BlitzenEngine::CameraContainer* pCameraContainer{ nullptr };
        BlitzenEngine::WVHOST m_worldVariableHost;
        BlitCL::BlitStack<BlitzenWorldUpdate, BlitzenCore::Ce_MaxWorldVariableCount> worldUpdates{};

        BlitzenEngine::Renderer P_RENDERER;
        BlitzenEngine::DrawContext m_drawContext;

        float deltaTime;

        inline WORLD_blit(BlitzenEngine::Camera& camera, BlitzenEngine::MeshResources& meshes, BlitzenEngine::RenderContainer& renders, 
            BlitzenEngine::TextureManager& textureManager, BlitzenPlatform::PlatformContext* pPlatform)
            :m_drawContext{ camera, meshes, renders, textureManager, pPlatform }
        {

        }
    };
}