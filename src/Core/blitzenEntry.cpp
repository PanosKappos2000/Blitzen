#include "Core/Events/blitEvents.h"
#include "Platform/blitPlatformContext.h"
#include "Platform/blitPlatform.h"
#include "BlitCL/blitSmartPointer.h"
#include "DbLog/blitAssert.h"
#include <thread>

using RndResourcesMemory = BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer>;
using WorldSystemMemory = BlitCL::SmartPointer<BlitzenWorld::BLITZEN_WORLD, BlitzenCore::AllocationType::Entity>;
using ControllerSystemMemory = BlitCL::SmartPointer<BlitzenCore::ControllerContainer, BlitzenCore::AllocationType::Engine>;

#if defined(BLIT_GDEV_EDT)
int main(int argc, char* argv[])
{
    /* ENGINE SYSTEMS INITIALIZATION */
    BlitzenWorld::BLITZEN_SYSTEM_CONTEXT SYSTEM{};
    SYSTEM.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::STARTUP;

    ControllerSystemMemory blitzenControllers;
    blitzenControllers.Make();
    SYSTEM.m_controllers = blitzenControllers->m_controllers;
	BlitzenCore::ZeroInitializeEventFunctionPointers(&SYSTEM);

    BlitzenCore::InitLogging();

    BlitzenCore::WorldTimeManager blitzenClock;
    SYSTEM.pClock = &blitzenClock;

    RndResourcesMemory renderingResources;
    renderingResources.Make();
    SYSTEM.pRenderingResources = renderingResources.Data();

    BlitzenPlatform::PlatformContext platform{};
    SYSTEM.pPlatform = &platform;

    WorldSystemMemory WORLD;
    WORLD.Make(renderingResources->m_meshContext, renderingResources->m_textureManager, &platform);
    WORLD->m_drawContext.m_pTerrain = &renderingResources->m_terrainContainer;
    WORLD->m_drawContext.m_pResidents = &WORLD->mResidents;
    WORLD->BMPR.Make();
    SYSTEM.pWORLD = WORLD.Data();

    BlitzenEngine::SetupCamera(SYSTEM.pWORLD->m_cameras[BlitzenCore::CE_INITIAL_CONTROLLER_ID]);
    BlitzenEngine::SetupCamera(SYSTEM.pWORLD->m_cameras[1]);

    BlitzenCore::Dasher dasher;
    SYSTEM.pDasher = &dasher;

    BlitzenPlatform::PlatformArgs platformArgs{&platform, &SYSTEM, WORLD->BMPR.Data(), &dasher};
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

#if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)
        // Using IMGUI for the editor requires some extra care for event handling
        BlitzenCore::UpdateInput(&SYSTEM, blitzenClock.mDeltaTime);
#else
        BlitzenCore::UpdateInput(&SYSTEM, blitzenClock.mDeltaTime);
#endif
    }
}

#else

#include "BlitzenMathLibrary/blitMLSIMD.h"

int main()
{
    BlitzenWorld::BLITZEN_SYSTEM_CONTEXT SYSTEM{};
    SYSTEM.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::STARTUP;

    ControllerSystemMemory blitzenControllers;
    blitzenControllers.Make();
    SYSTEM.m_controllers = blitzenControllers->m_controllers;
    BlitzenCore::ZeroInitializeEventFunctionPointers(&SYSTEM);

    BlitzenCore::InitLogging();

    BlitzenCore::WorldTimeManager blitzenClock;
    SYSTEM.pClock = &blitzenClock;

    blitzenClock.Startup();

    BlitzenCore::BlitPerformanceCounter counter;
    counter.Generate(&blitzenClock);

    constexpr uint32_t loopCount = 100'000'000;

    BlitML::mat4 testMatrix = BlitML::Translate(BlitML::float3{ 1.0f, 2.0f, 3.0f });
    BlitML::float4 testVector = BlitML::float4(1.0f, 2.0f, 3.0f, 1.0f);
    BlitML::float4 result;
    volatile float accumulator = 0.0f;

    double conventionalStart = counter.Startup();

    for (uint32_t i = 0; i < loopCount; ++i)
    {
        result = testMatrix * testVector;
        accumulator = result.x;
    }

    double conventionalTime = counter.End();

    BLIT_FATAL("Conventional counter: %6f", conventionalTime);

    counter.Reset();

    double SIMDStart = counter.Startup();

    for (uint32_t i = 0; i < loopCount; ++i)
    {
        result = BCPSS::MulMat4Vec4(testMatrix, testVector);
        accumulator = result.x;
    }

    double SIMDEnd = counter.End();

    BLIT_FATAL("SIMD counter: %6f", SIMDEnd);

    return 0;
}
#endif


//Assets/Scenes/CityLow/scene.gltf ../../GltfTestScenes/Scenes/Plaza/scene.gltf ../../GltfTestScenes/Scenes/Museum/scene.gltf (personal test scenes for copy pasting)