#include "Core/Events/blitEvents.h"
#include "Platform/blitPlatformContext.h"
#include "Platform/blitPlatform.h"
#include "BlitCL/blitSmartPointer.h"
#include "DbLog/blitAssert.h"
#include <thread>

using RndResourcesMemory = BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer>;
using ComponentSystemMemory = BlitCL::SmartPointer<BlitzenEngine::ComponentSystem, BlitzenCore::AllocationType::Entity>;
using WorldSystemMemory = BlitCL::SmartPointer<BlitzenWorld::WORLD_blit, BlitzenCore::AllocationType::Entity>;

#if defined(BLIT_GDEV_EDT)
int main(int argc, char* argv[])
{
    /* ENGINE SYSTEMS INITIALIZATION */
    BlitzenWorld::BLITZEN_SYSTEM_CONTEXT blitzenPrivateContext{};
    blitzenPrivateContext.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::LOADING;

    BlitzenCore::InitLogging();

    BlitzenEngine::CameraContainer blitzenCameraSystem;
    auto& mainCamera = blitzenCameraSystem.GetMainCamera();
    BlitzenEngine::SetupCamera(mainCamera);

    BlitzenCore::WorldTimeManager blitzenClock;
    blitzenPrivateContext.pClock = &blitzenClock;

    ComponentSystemMemory blitzenComponentSystem;
    blitzenComponentSystem.Make();
    blitzenPrivateContext.pComponents = blitzenComponentSystem.Data();

    RndResourcesMemory renderingResources;
    renderingResources.Make();
    blitzenPrivateContext.pRenderingResources = renderingResources.Data();

    BlitzenPlatform::PlatformContext platform{};
    blitzenPrivateContext.pPlatform = &platform;

    WorldSystemMemory WORLD;
    WORLD.Make(mainCamera, renderingResources->m_meshContext, renderingResources->m_textureManager, &platform);
    WORLD->pCameraContainer = &blitzenCameraSystem;
    WORLD->m_drawContext.m_pResidents = &WORLD->m_residents;
    WORLD->P_RENDERER.Make();
    blitzenPrivateContext.pWORLD = WORLD.Data();
    
    BlitzenCore::EventSystemMemory blitzenEventSystem;
    blitzenEventSystem.Make(WORLD.Data(), std::ref(blitzenPrivateContext));

    BlitzenCore::Dasher dasher;
    blitzenPrivateContext.pDasher = &dasher;

    BlitzenPlatform::PlatformArgs platformArgs{&platform, blitzenEventSystem.Data(), WORLD->P_RENDERER.Data(), &dasher};
    BLIT_ASSERT(BlitzenPlatform::SystemStartup(platformArgs));

    // LOADING RESOURCES
    std::thread loadingThread
    {
        [&]()
        {
            BlitzenWorld::LoadingLoop(argc, argv, blitzenPrivateContext, WORLD->m_drawContext);
        }
    };
    #if(_WIN32)
        loadingThread.detach();
    #else
        loadingThread.join();
    #endif

    // LOOP
    while(blitzenPrivateContext.BLITZEN_ENGINE.m_state != BlitzenCore::EngineState::SHUTDOWN)
    {
        BlitzenWorld::WorldLoop(blitzenPrivateContext);

        BlitzenWorld::UpdateLoop(blitzenPrivateContext);

        BlitzenWorld::RenderLoop(blitzenPrivateContext);

#if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)
        // Using IMGUI for the editor requires some extra care for event handling
        blitzenEventSystem->UpdateInput(blitzenClock.m_deltaTime, &dasher.m_eventContext);
#else
        blitzenEventSystem->UpdateInput(blitzenClock.m_deltaTime);
#endif
    }
}

#else

int main()
{
    return 0;
}
#endif


//Assets/Scenes/CityLow/scene.gltf ../../GltfTestScenes/Scenes/Plaza/scene.gltf ../../GltfTestScenes/Scenes/Museum/scene.gltf (personal test scenes for copy pasting)