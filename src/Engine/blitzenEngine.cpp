#include "Engine/blitzenEngine.h"
#include "Platform/platform.h"
#include "Renderer/blitRenderer.h"
#include "Core/blitzenCore.h"
#include "Core/blitEvents.h"
#include "Game/blitCamera.h"

namespace BlitzenEngine
{
    // Renderer type resolution. TODO: Move this to renderer.h, so that more things have access to it
    #if defined(BLIT_GL_LEGACY_OVERRIDE) && defined(BLITZEN_VULKAN_OVERRIDE) && defined(_WIN32)
        using Renderer = 
        BlitCL::SmartPointer<BlitzenGL::OpenglRenderer, BlitzenCore::AllocationType::Renderer>;
    #elif defined(BLITZEN_VULKAN_OVERRIDE) && defined(_WIN32)
        using Renderer = 
        BlitCL::SmartPointer<BlitzenDX12::Dx12Renderer, BlitzenCore::AllocationType::Renderer>;
    #else
        using Renderer = 
        BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>;
    #endif

    // Function that is defined in blitzenDefaultEvents.cpp and only called once in the Run function
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
        // These systems can be shutdown after the memory manager for now
        BlitzenCore::ShutdownLogging();
        BlitzenPlatform::PlatformShutdown();
    }
}

int main(int argc, char* argv[])
{
    BlitzenEngine::Engine engine;

    // Memory manager should be the first system initialized and destroyed after anything that allocates memory 
    // *The exceptions are the engine class and the logging and platform systems, which make no dyncamic allocations anyway
    BlitzenCore::MemoryManagerState blitzenMemory;

    // Initializes logging. This always succeeds for now
    BlitzenCore::InitLogging();

    // Initializes the camera stystem
    BlitzenEngine::CameraSystem cameraSystem;

    // Initializes the event system as a smart pointer
    BlitCL::SmartPointer<BlitzenCore::EventSystemState> eventSystemState;

    // Initializes the input system after the event system
    BlitCL::SmartPointer<BlitzenCore::InputSystemState> inputSystemState;

    // Platform specific code initalization. 
    // This should be called after the event system has been initialized because the event function is called.
    // That will break the application without the event system.
    BLIT_ASSERT(BlitzenPlatform::PlatformStartup(BlitzenEngine::ce_blitzenVersion))
            
    // With the event and input systems active, register the engine's default events and input bindings
    BlitzenEngine::RegisterDefaultEvents();

    uint8_t bRenderingSystem = 0;
    BlitzenEngine::Renderer renderer;

    if(renderer->Init(BlitzenEngine::ce_initialWindowWidth, BlitzenEngine::ce_initialWindowHeight))
        bRenderingSystem = 1;
    else
    {
        BLIT_FATAL("Failed to initialize rendering API")
        bRenderingSystem = 0;
    }
        
    // Allocates the rendering resources
    BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer> pResources;
    LoadRenderingResourceSystem(pResources.Data());

    // Setup the main camera
    BlitzenEngine::Camera& mainCamera = cameraSystem.GetCamera();
    SetupCamera(mainCamera, BlitML::Radians(BlitzenEngine::ce_initialFOV), 
    static_cast<float>(BlitzenEngine::ce_initialWindowWidth), 
    static_cast<float>(BlitzenEngine::ce_initialWindowHeight), 
    BlitzenEngine::ce_znear, 
    BlitML::vec3(BlitzenEngine::ce_initialCameraX, BlitzenEngine::ce_initialCameraY, BlitzenEngine::ce_initialCameraZ), 
    BlitzenEngine::ce_initialDrawDistance);

    // Checks for command line arguments
    if(argc > 1)
    {
        // If the first command line argument is rendring stress test, the rendering stress test is loaded
        if(strcmp(argv[1], "RenderingStressTest") == 0)
        {
            constexpr uint32_t ce_defaultObjectCount = 1'000'000;
            LoadGeometryStressTest(pResources.Data(), ce_defaultObjectCount);

            // The following arguments are used as gltf filepaths
            for(int32_t i = 2; i < argc; ++i)
            {
                LoadGltfScene(pResources.Data(), argv[i]);
            }
        }
        // Else, all arguments are used as gltf filepaths
        else
        {
            for(int32_t i = 1; i < argc; ++i)
            {
                LoadGltfScene(pResources.Data(), argv[i]);
            }
        }
    }

    // Sets the draw count to the render object count   
    uint32_t drawCount = pResources.Data()->renderObjectCount;

    // Uploads the textures from the filepaths that were saved, TODO: Put this back to the renderer
    for (uint32_t i = 0; i < pResources->textureCount; ++i)
    {
        BlitzenEngine::TextureStats& texture = pResources->textures[i];
        BlitzenEngine::DDS_HEADER header{};
        BlitzenEngine::DDS_HEADER_DXT10 header10{};
        renderer->UploadTexture(header, header10, texture.pTextureData, texture.filepath.c_str());
    }

    // Passes the resources that were loaded to the renderer
    if(!bRenderingSystem || !renderer->SetupForRendering(pResources.Data(), 
    mainCamera.viewData.pyramidWidth, mainCamera.viewData.pyramidHeight))
    {
        BLIT_FATAL("Renderer failed to setup, Blitzen's rendering system is offline")
        bRenderingSystem = 0;
    }
        
    // Starts the clock
    double clockStartTime = BlitzenPlatform::PlatformGetAbsoluteTime();
    double clockElapsedTime = 0;
    double previousTime = clockElapsedTime;// Initialize previous frame time to the elapsed time

    // Main Loop starts
    while(engine.IsRunning())
    {
        // Captures events
        if(!BlitzenPlatform::PlatformPumpMessages())
        {
            engine.Shutdown();
        }

        if(engine.IsActive())
        {
            // Get the elapsed time of the application
            clockElapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - clockStartTime;
            // Update the delta time by using the previous elapsed time
            engine.SetDeltaTime(clockElapsedTime - previousTime);
            // Update the previous elapsed time to the current elapsed time
            previousTime = clockElapsedTime;

            // With delta time retrieved, call update camera to make any necessary changes to the scene based on its transform
            UpdateCamera(mainCamera, static_cast<float>(engine.GetDeltaTime()));

            // Draws the frame
            if(bRenderingSystem)
            {
                BlitzenEngine::DrawContext drawContext(&mainCamera, drawCount);
                renderer->DrawFrame(drawContext);
            }

            // Make sure that the window resize is set to false after the renderer is notified
            mainCamera.transformData.windowResize = 0;

            BlitzenCore::UpdateInput(engine.GetDeltaTime());
        }
    }

    // I want to remove this eventually and replace it with RAII destructors for every renderer type
    renderer->Shutdown();

    engine.BeginShutdown();
}
//Assets/Scenes/CityLow/scene.gltf ../../GltfTestScenes/Scenes/Plaza/scene.gltf ../../GltfTestScenes/Scenes/Museum/scene.gltf (personal test scenes for copy+paste)