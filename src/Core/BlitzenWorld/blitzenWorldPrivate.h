#pragma once
#include "Core/Dasher/Interface/dasherInterface.h"
#include "blitzenWorld.h"
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
        BlitzenEngine::RenderingResources* pRenderingResources{ nullptr };
        BlitzenEngine::EntityManager* pEntityMangager{ nullptr };
        BlitzenPlatform::PlatformContext* pPlatform{ nullptr };
        BlitzenCore::Dasher* pDasher{ nullptr };
        BlitzenCore::WorldTimeManager* pClock;

        WORLD_blit* pWORLD{ nullptr };
    };

    void LoadingLoop(int argc, char** argv, BlitzenPrivateContext& context, BlitzenEngine::DrawContext& drawContext);

    void RenderLoop(BlitzenPrivateContext& context);

    void UpdateLoop(BlitzenPrivateContext& context);

    void WorldLoop(BlitzenPrivateContext& context);

    bool ManageGltf(const char* filepath, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::EntityManager* pManager, BlitzenEngine::RendererPtrType pRenderer, 
        BlitzenEngine::SceneContext* pScene = nullptr);

    void AllocateWorldVariables(BlitzenEngine::WV_CREATE_CONTEXT* worldVarArray, uint32_t contextCount, WORLD_blit* pWORLD);

    void LoadGeometryStressTest(BlitzenEngine::RenderContainer& renders, BlitzenEngine::MeshResources& meshContext, float transformMultiplier, BlitzenEngine::SceneContext* pScene = nullptr);

    bool CreateSceneFromArguments(int argc, char** argv, BlitzenEngine::RenderingResources* pResources, WORLD_blit* pWORLD, BlitzenEngine::EntityManager* pManager);

    void S_WORLD_UPDATE_RECEIVER_SEND_TRANSFORM(BlitzenEngine::DynamicTransform* pTransform, uint32_t wvID);
}