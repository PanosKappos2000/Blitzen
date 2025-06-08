#pragma once
#include "Renderer/Interface/blitRenderer.h"
#include "Core/Dasher/Interface/dasherInterface.h"
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
        BlitzenEngine::WORLD_blit* pWORLD{ nullptr };
        BlitzenEngine::RenderingResources* pRenderingResources{ nullptr };
        BlitzenEngine::EntityManager* pEntityMangager{ nullptr };
        BlitzenPlatform::PlatformContext* pPlatform{ nullptr };
        BlitzenCore::Dasher* pDasher{ nullptr };

        std::mutex m_resourceLoadingMutex;
        std::condition_variable m_resourceLoadingFinishedConditional;
        std::atomic<bool> m_loadingDoneAtomic{ false };

        void* pBlitzenContext;
    };

    void LoadingLoop(int argc, char** argv, BlitzenPrivateContext& context, BlitzenEngine::DrawContext& drawContext);
}