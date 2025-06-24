#pragma once
#include "BlitCL/blitPfn.h"
#include "Renderer/Scene/blitScene.h"

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
    class WORLD_blit
    {
    public:

        // Scene resources
        BlitzenEngine::SceneContext m_scenes[10]{};
        uint32_t m_sceneCount{ 0 };

        // Camera should be component. Does not belong here. The container is also a dumb idea. The need for an extra camera should just give rise to a new system
        BlitzenEngine::CameraContainer* pCameraContainer{ nullptr };

        // This will be moved to a renderer manager.
        BlitzenEngine::Renderer P_RENDERER{};
        BlitzenEngine::DrawContext m_drawContext;

        // World residents
        BlitzenEngine::WORLD_RESIDENTS m_residents{};

        BlitzenEngine::WVHOST m_worldVariables{};

        float deltaTime{0.f};

        inline WORLD_blit(BlitzenEngine::Camera& camera, BlitzenEngine::MeshResources& meshes, BlitzenEngine::TextureManager& textureManager, BlitzenPlatform::PlatformContext* pPlatform)
            :m_drawContext{ camera, meshes, textureManager, pPlatform }
        {

        }
    };

    bool RenderingResourcesInit(BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer);


}