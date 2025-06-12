#pragma once
#include "Core/Dasher/Interface/dasherInterface.h"
#include "blitzenWorld.h"
#include "Renderer/Entities/Interface/blitComponents.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace BlitzenWorld
{
    struct BlitzenPrivateContext
    {
        // SYSTEMS
        BlitzenCore::EngineState* pEngineState{ nullptr };
        BlitzenEngine::RenderingResources* pRenderingResources{ nullptr };
        BlitzenEngine::ComponentSystem* pComponents{ nullptr };
        BlitzenPlatform::PlatformContext* pPlatform{ nullptr };
        BlitzenCore::Dasher* pDasher{ nullptr };
        BlitzenCore::WorldTimeManager* pClock;

        WORLD_blit* pWORLD{ nullptr };
    };

    void LoadingLoop(int argc, char** argv, BlitzenPrivateContext& context, BlitzenEngine::DrawContext& drawContext);

    void RenderLoop(BlitzenPrivateContext& context);

    void UpdateLoop(BlitzenPrivateContext& context);

    void WorldLoop(BlitzenPrivateContext& context);

    void S_WORLD_UPDATE_RESIDENT_MOVED(uint32_t id);
}