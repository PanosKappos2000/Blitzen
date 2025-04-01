#include "Engine/blitzenEngine.h"

// Single file gltf loading https://github.com/jkuhlmann/cgltf
// Placed here because cgltf is called from a template function
#define CGLTF_IMPLEMENTATION

#include "Platform/platform.h"
#include "Core/blitzenCore.h"
#include "Core/blitEvents.h"
#include "Renderer/blitRenderer.h"
#include "Game/blitCamera.h"
#include "Core/blitTimeManager.h"

#include <thread>

namespace BlitzenEngine
{
    // Function that is defined in blitzenDefaultEvents.cpp and only called once in main
    void RegisterDefaultEvents();

    Engine* Engine::s_pEngine = nullptr;

    Engine::Engine() : m_bRunning{ true }, m_bSuspended{ true }
    {
        BLIT_ASSERT(!s_pEngine);
        s_pEngine = this;
    }

    Engine::~Engine()
    {
        // This does absolutely nothing right now
        BlitzenCore::ShutdownLogging();
    }
}

using EventSystem = BlitCL::SmartPointer<BlitzenCore::EventSystemState>;
using InputSystem = BlitCL::SmartPointer<BlitzenCore::InputSystemState>;
using PRenderingResources = BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer>;
using Entities = BlitCL::SmartPointer<BlitzenEngine::GameObjectManager, BlitzenCore::AllocationType::Entity>;

static void LoadRenderingResources(int argc, char** argv,
    BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer,
    BlitzenEngine::GameObjectManager* pManager, uint8_t& bOnpc,
    BlitzenEngine::Camera& camera, bool& bRenderingSystem)
{
    BlitzenEngine::CreateSceneFromArguments(argc, argv, pResources,
        pRenderer, pManager, bOnpc);
    if (!bRenderingSystem ||
        !pRenderer->SetupForRendering(pResources, camera.viewData.pyramidWidth,
            camera.viewData.pyramidHeight
        ))
    {
        BLIT_FATAL("Renderer failed to setup, Blitzen's rendering system is offline")
            bRenderingSystem = false;
    }

    BlitzenEngine::Engine::GetEngineInstancePointer()->ReActivate();
}


int main(int argc, char* argv[])
{
    /*
        Engine Systems initialization
    */

    BlitzenEngine::Engine engine;
    
    // Memory manager, needs to be created before everything that uses heap allocations
    BlitzenCore::MemoryManagerState blitzenMemory;

    BlitzenCore::InitLogging();
    
    // Holds all available cameras
    BlitzenEngine::CameraSystem cameraSystem;
    
    EventSystem eventSystemState;
	eventSystemState.Make();
    InputSystem inputSystemState;
    inputSystemState.Make();

    // Platform specific code + window creation
    BLIT_ASSERT(BlitzenPlatform::PlatformStartup(BlitzenEngine::ce_blitzenVersion))
            
    BlitzenEngine::RegisterDefaultEvents();

    // Initial renderer setup (API initialization, see blitRenderer.h for API selection)
    bool bRenderingSystem = false;
    BlitzenEngine::Renderer renderer;
    renderer.Make();
    if(renderer->Init(BlitzenEngine::ce_initialWindowWidth, BlitzenEngine::ce_initialWindowHeight))
        bRenderingSystem = true;
    else
    {
        BLIT_FATAL("Failed to initialize rendering API")
        bRenderingSystem = false;
    }
        
    PRenderingResources renderingResources;
    renderingResources.Make(renderer.Data());
    
    // Initial camera
    BlitzenEngine::Camera& mainCamera = cameraSystem.GetCamera();
    BlitzenEngine::SetupCamera(mainCamera);

    Entities entities;
	entities.Make();

    // Loading resources
    uint8_t bOnpc = 0;
#if defined(NDEBUG)
    
    std::thread loadingThread{ [&]() {
        LoadRenderingResources(argc, argv, renderingResources.Data(), renderer.Data(),
            entities.Data(), bOnpc, mainCamera, bRenderingSystem);
    }};
    loadingThread.detach();
#else
    LoadRenderingResources(argc, argv, renderingResources.Data(), renderer.Data(),
        entities.Data(), bOnpc, mainCamera, bRenderingSystem);
#endif
    



    /*
        Main loop starts here
    */
    BlitzenCore::WorldTimerManager coreClock;
    while(engine.IsRunning())
    {
        // Captures events
        if(!BlitzenPlatform::PlatformPumpMessages())
        {
            engine.Shutdown();
        }

        if(engine.IsActive())
        {
            coreClock.Update();

            UpdateCamera(mainCamera, static_cast<float>(coreClock.GetDeltaTime()));

			entities->UpdateDynamicObjects();

            if(bRenderingSystem)
            {
                BlitzenEngine::DrawContext drawContext(&mainCamera, renderingResources.Data(), 
                    bOnpc // Tells the renderer if oblique near-plane clipping objects exist
                );
                renderer->DrawFrame(drawContext);// This is a bit important
            }

            // Reset window resize, TODO: Why is this here??????
            mainCamera.transformData.windowResize = 0;
        }

        BlitzenCore::UpdateInput(coreClock.GetDeltaTime());
    }
    engine.BeginShutdown();

    // Destroys window before other destructors, otherwise it will lag
    // (because it waits for a huge amount of deletes)
    BlitzenPlatform::PlatformShutdown();
}


//Assets/Scenes/CityLow/scene.gltf ../../GltfTestScenes/Scenes/Plaza/scene.gltf ../../GltfTestScenes/Scenes/Museum/scene.gltf (personal test scenes for copy pasting)