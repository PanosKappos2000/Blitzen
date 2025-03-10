#include "Engine/blitzenEngine.h"
#include "Platform/platform.h"
#include "Renderer/blitRenderer.h"
#include "Core/blitzenCore.h"
#include "Core/blitEvents.h"
#include "Game/blitCamera.h"

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

int main(int argc, char* argv[])
{
    // Not as important as it sounds
    BlitzenEngine::Engine engine;

    // Only systems that do not allocate on the heap may be initialized before here
    // The opposite goes for destruction
    BlitzenCore::MemoryManagerState blitzenMemory;

    // Logging system. Extremely simplistic for now
    BlitzenCore::InitLogging();

    // Camera system. Holds a fixed amount of cameras that is known at compile time
    BlitzenEngine::CameraSystem cameraSystem;

    // Event handling
    EventSystem eventSystemState;
	eventSystemState.Make();
    InputSystem inputSystemState;
    inputSystemState.Make();

    // Platform specific code. Mostly window creation. Lightweight, no allocations, but needs to wait for event systems
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
        
    // Allocates the rendering resources
    RenderingResources renderingResources;
	renderingResources.Make();
    LoadRenderingResourceSystem(renderingResources.Data(), renderer.Data());

    // Game object manager, holds all client objects with game logic
    BlitzenEngine::GameObjectManager gameObjectManager;
	gameObjectManager.AddObject<BlitzenEngine::GameObject>("undefined", 1);

    // Main camera
    BlitzenEngine::Camera& mainCamera = cameraSystem.GetCamera();
    SetupCamera(mainCamera, 
        BlitML::Radians(BlitzenEngine::ce_initialFOV), // field of view
        static_cast<float>(BlitzenEngine::ce_initialWindowWidth), 
        static_cast<float>(BlitzenEngine::ce_initialWindowHeight), 
        BlitzenEngine::ce_znear, // znear
        /* Camera position vector */
        BlitML::vec3(BlitzenEngine::ce_initialCameraX, // x
        BlitzenEngine::ce_initialCameraY, /*y*/ BlitzenEngine::ce_initialCameraZ), // z
        /* Camera position vector (parameter end)*/ 
        BlitzenEngine::ce_initialDrawDistance // the engine uses infinite projection matrix by default, so the draw distance is the zfar
    );

    // Uses the command line arguments to load resources for the renderer
    uint8_t bOnpc = 0;
    BlitzenEngine::CreateSceneFromArguments(argc, argv, renderingResources.Data(), renderer.Data(), bOnpc);

    // Rendering setup with the loaded resources. Checks if it fails first
    if(!bRenderingSystem || // Checks if API initialization was succesful
        // Then tries the function
        !renderer->SetupForRendering( 
            renderingResources.Data(), 
            mainCamera.viewData.pyramidWidth, 
            mainCamera.viewData.pyramidHeight
    ))
    {
        BLIT_FATAL("Renderer failed to setup, Blitzen's rendering system is offline")
        bRenderingSystem = 0;
    }
        
    // The clock is used to keep movement stable with variable fps
    // At the moment, it is not doing its job actually
    double clockStartTime = BlitzenPlatform::PlatformGetAbsoluteTime();
    double clockElapsedTime = 0;// No time has passed
    double previousTime = clockElapsedTime;// Holds the previous frame time

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
            // Updates delta time
            clockElapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - clockStartTime;
            engine.SetDeltaTime(clockElapsedTime - previousTime);
            previousTime = clockElapsedTime;

            UpdateCamera(mainCamera, static_cast<float>(engine.GetDeltaTime()));

			gameObjectManager.UpdateDynamicObjects();

            if(bRenderingSystem)
            {
                BlitzenEngine::DrawContext drawContext(
                    &mainCamera, 
                    renderingResources.Data(), 
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