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

namespace BlitzenEngine
{
    // Function that is defined in blitzenDefaultEvents.cpp and only called once in main
    void RegisterDefaultEvents();

    Engine* Engine::s_pEngine = nullptr;

    Engine::Engine()
    {
        // Starts the engine if this is the first time that the constructor gets called
        BLIT_ASSERT(!s_pEngine);
        s_pEngine = this;
        BootUp();
    }

    Engine::~Engine()
    {
        // This does absolutely nothing right now
        BlitzenCore::ShutdownLogging();
    }
}

using EventSystem = BlitCL::SmartPointer<BlitzenCore::EventSystemState>;
using InputSystem = BlitCL::SmartPointer<BlitzenCore::InputSystemState>;
using RenderingResources = BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer>;
using Entities = BlitCL::SmartPointer<BlitzenEngine::GameObjectManager, BlitzenCore::AllocationType::Entity>;

int main(int argc, char* argv[])
{
    // Not as important as it looks
    BlitzenEngine::Engine engine;
    // Only systems that do not allocate on the heap may be initialized before here
    // The opposite goes for destruction
    BlitzenCore::MemoryManagerState blitzenMemory;

    // Logging system. Extremely simplistic for now
    BlitzenCore::InitLogging();
    // Camera container
    BlitzenEngine::CameraSystem cameraSystem;
    // Events
    EventSystem eventSystemState;
	eventSystemState.Make();
    // Inputs
    InputSystem inputSystemState;
    inputSystemState.Make();

    // Platform specific code + window creation
    BLIT_ASSERT(BlitzenPlatform::PlatformStartup(BlitzenEngine::ce_blitzenVersion))
            
    // Default engine events (see blitzenDefaultEvents.cpp). The client app may modify these with some exceptions
    BlitzenEngine::RegisterDefaultEvents();

    // Initial renderer setup (API initialization, see blitRenderer.h for API selection)
    uint8_t bRenderingSystem = 0;
    BlitzenEngine::Renderer renderer;
    renderer.Make();
    if(renderer->Init(BlitzenEngine::ce_initialWindowWidth, BlitzenEngine::ce_initialWindowHeight))
        bRenderingSystem = 1;
    else
    {
        BLIT_FATAL("Failed to initialize rendering API")
        bRenderingSystem = 0;
    }
        
    // Rendering resources
    RenderingResources renderingResources;
    renderingResources.Make(renderer.Data());

    // Main camera
    BlitzenEngine::Camera& mainCamera = cameraSystem.GetCamera();
    BlitzenEngine::SetupCamera(mainCamera);

    // Game object manager
    Entities entities;
	entities.Make();

    // Command line arguments
    uint8_t bOnpc = 0;
    BlitzenEngine::CreateSceneFromArguments(argc, argv, renderingResources.Data(), 
        renderer.Data(), entities.Data(), bOnpc);

    // Rendering setup with the loaded resources
    if(!bRenderingSystem || // Checks if API initialization was succesful first
        !renderer->SetupForRendering(renderingResources.Data(), mainCamera.viewData.pyramidWidth, 
            mainCamera.viewData.pyramidHeight
    ))
    {
        BLIT_FATAL("Renderer failed to setup, Blitzen's rendering system is offline")
        bRenderingSystem = 0;
    }
        
    BlitzenCore::WorldTimerManager coreClock;

    // MAIN LOOP
    while(engine.IsRunning())
    {
        // Captures events
        if(!BlitzenPlatform::PlatformPumpMessages())
        {
            engine.Shutdown();
        }

        if(engine.IsActive())
        {
            // Updates delta time(TODO: Remove deltaTime from engine)
            engine.SetDeltaTime(coreClock.GetDeltaTime());

            UpdateCamera(mainCamera, static_cast<float>(engine.GetDeltaTime()));

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

            BlitzenCore::UpdateInput(engine.GetDeltaTime());
        }
    }
    engine.BeginShutdown();

    // Destroys window before other destructors, otherwise it will lag
    // (because it waits for a huge amount of deletes)
    BlitzenPlatform::PlatformShutdown();
}


//Assets/Scenes/CityLow/scene.gltf ../../GltfTestScenes/Scenes/Plaza/scene.gltf ../../GltfTestScenes/Scenes/Museum/scene.gltf (personal test scenes for copy pasting)