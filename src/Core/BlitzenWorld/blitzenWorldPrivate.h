#pragma once
#include "Renderer/Interface/blitRenderer.h"
#include "Core/Dasher/Interface/dasherInterface.h"
#include "Renderer/Entities/Interface/blitEntityInterface.h"
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

    bool ManageGltf(const char* filepath, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::EntityManager* pManager, BlitzenEngine::RendererPtrType pRenderer, 
        BlitzenEngine::SceneContext* pScene = nullptr);

    void CreateDynamicObjectRendererTest(BlitzenEngine::RenderContainer& renders, BlitzenEngine::MeshResources& meshes, BlitzenEngine::EntityManager* pManager, BlitzenEngine::SceneContext* pScene = nullptr);

    void LoadGeometryStressTest(BlitzenEngine::RenderContainer& renders, BlitzenEngine::MeshResources& meshContext, float transformMultiplier, BlitzenEngine::SceneContext* pScene = nullptr);

    bool CreateSceneFromArguments(int argc, char** argv, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::WORLD_blit* pWORLD, BlitzenEngine::EntityManager* pManager);
}