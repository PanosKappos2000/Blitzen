#pragma once
#include "Core/Dasher/Interface/dasherInterface.h"
#include "Renderer/WORLD/blitzenWorld.h"
#include "Renderer/Entities/Interface/blitComponents.h"

namespace BlitzenWorld
{
    struct BLITZEN_SYSTEM_CONTEXT
    {
        BlitzenCore::Engine BLITZEN_ENGINE;
        
        // SYSTEMS
        BlitzenEngine::RenderingResources* pRenderingResources{ nullptr };
        BlitzenEngine::ComponentSystem* pComponents{ nullptr };
        BlitzenPlatform::PlatformContext* pPlatform{ nullptr };
        BlitzenCore::Dasher* pDasher{ nullptr };
        BlitzenCore::WorldTimeManager* pClock;

        WORLD_blit* pWORLD{ nullptr };
    };

    void LoadingLoop(int argc, char** argv, BLITZEN_SYSTEM_CONTEXT& context, BlitzenEngine::DrawContext& drawContext);

    void RenderLoop(BLITZEN_SYSTEM_CONTEXT& context);

    void UpdateLoop(BLITZEN_SYSTEM_CONTEXT& context);

    void WorldLoop(BLITZEN_SYSTEM_CONTEXT& context);

    void S_WORLD_UPDATE_RESIDENT_MOVED(uint32_t id);
}