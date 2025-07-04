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
    BlitzenWorld::BLITZEN_SYSTEM_CONTEXT SYSTEM{};
    SYSTEM.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::STARTUP;
	BlitzenCore::ZeroInitializeEventFunctionPointers(&SYSTEM);

    BlitzenCore::InitLogging();

    BlitzenCore::WorldTimeManager blitzenClock;
    SYSTEM.pClock = &blitzenClock;

    ComponentSystemMemory blitzenComponentSystem;
    blitzenComponentSystem.Make();
    SYSTEM.pComponents = blitzenComponentSystem.Data();

    RndResourcesMemory renderingResources;
    renderingResources.Make();
    SYSTEM.pRenderingResources = renderingResources.Data();

    BlitzenPlatform::PlatformContext platform{};
    SYSTEM.pPlatform = &platform;

    WorldSystemMemory WORLD;
    WORLD.Make(renderingResources->m_meshContext, renderingResources->m_textureManager, &platform);
    WORLD->m_drawContext.m_pResidents = &WORLD->m_residents;
    WORLD->P_RENDERER.Make();
    SYSTEM.pWORLD = WORLD.Data();

    BlitzenEngine::SetupCamera(SYSTEM.pWORLD->m_cameras[BlitzenCore::CE_INITIAL_CONTROLLER_ID]);
    BlitzenEngine::SetupCamera(SYSTEM.pWORLD->m_cameras[1]);

    BlitzenCore::Dasher dasher;
    SYSTEM.pDasher = &dasher;

    BlitzenPlatform::PlatformArgs platformArgs{&platform, &SYSTEM, WORLD->P_RENDERER.Data(), &dasher};
    BLIT_ASSERT(BlitzenPlatform::SystemStartup(platformArgs));

    // LOADING RESOURCES
    std::thread loadingThread
    {
        [&]()
        {
            BlitzenWorld::LoadingLoop(argc, argv, SYSTEM, WORLD->m_drawContext);
        }
    };
    #if(_WIN32)
        loadingThread.detach();
    #else
        loadingThread.join();
    #endif

    blitzenClock.Startup();

    // LOOP
    while(SYSTEM.BLITZEN_ENGINE.m_state != BlitzenCore::EngineState::SHUTDOWN)
    {
        BlitzenWorld::WorldLoop(SYSTEM);

        BlitzenWorld::BMPR_DRIVE(SYSTEM);

        BlitzenWorld::WV_DRIVE(SYSTEM);

#if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)
        // Using IMGUI for the editor requires some extra care for event handling
        BlitzenCore::UpdateInput(&SYSTEM, blitzenClock.m_deltaTime, &dasher.m_eventContext);
#else
        BlitzenCore::UpdateInput(&SYSTEM, blitzenClock.m_deltaTime);
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