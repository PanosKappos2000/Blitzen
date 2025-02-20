/* 
    The code in this file calls all of the functions needed to run the applications.
    Contains the main function at the bottom
*/
 
#include "Engine/blitzenEngine.h"
#include "Platform/platform.h"
#include "Renderer/blitRenderer.h"
#include "Core/blitzenCore.h"
#include "Core/blitEvents.h"
#include "Game/blitCamera.h"

namespace BlitzenEngine
{
    // Static member variable needs to be declared in the .cpp file as well
    Engine* Engine::s_pEngine;

    Engine::Engine()
    {
        // There should not be a 2nd instance of Blitzen Engine
        if(GetEngineInstancePointer())
        {
            BLIT_ERROR("Blitzen is already active")
            return;
        }

        // Initialize the engine if it is the first time the constructor is called
        else
        {
            // Initalize the instance and the system boolean to avoid creating or destroying a 2nd instance
            s_pEngine = this;
            BLIT_INFO("%s Booting", ce_blitzenVersion)
        }
    }



    // Everything besides the engine itself lives inside this scope
    void Engine::Run(uint32_t argc, char* argv[])
    {
        BlitzenCore::MemoryManagerState blitzenMemory;

        // Initialize logging
        BlitzenCore::InitLogging();

        // Initialize the camera stystem
        BlitzenEngine::CameraSystem cameraSystem;

        // Initialize the event system as a smart pointer
        BlitCL::SmartPointer<BlitzenCore::EventSystemState> eventSystemState;

        // Initialize the input system after the event system
        BlitCL::SmartPointer<BlitzenCore::InputSystemState> inputSystemState;

        // Platform specific code initalization. 
        // This should be called after the event system has been initialized because the event function is called.
        // That will break the application without the event system.
        BLIT_ASSERT(BlitzenPlatform::PlatformStartup(ce_blitzenVersion))
            
        // With the event and input systems active, register the engine's default events and input bindings
        RegisterDefaultEvents();

        // Rendering system holds all API specific renderer (Vulkan and Opengl for now)
        BlitCL::SmartPointer<RenderingSystem, BlitzenCore::AllocationType::Renderer> renderer;
        
        // Allocated the rendering resources on the heap, it is too big for the stack of this function
        BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer> pResources;
        
        // If the engine passes the above assertion, then it means that it can run the main loop (unless some less fundamental stuff makes it fail)
        bRunning = 1;
        bSuspended = 0;

        // Setup the main camera
        Camera& mainCamera = cameraSystem.GetCamera();
        SetupCamera(mainCamera, BlitML::Radians(ce_initialFOV), 
        static_cast<float>(ce_initialWindowWidth), static_cast<float>(ce_initialWindowHeight), 
        ce_znear, BlitML::vec3(ce_initialCameraX, ce_initialCameraY, ce_initialCameraZ), ce_initialDrawDistance);

        // Loads obj meshes that will be drawn with "random" transforms by the millions to stress test the renderer
        // If BLITZEN_RENDERING_STRESS_TEST is defined, this number will be ignored. 
        // The renderer will create an amount of objects close to the max allowed
        uint32_t drawCount = 1'000'000;
        LoadGeometryStressTest(pResources.Data(), drawCount, renderer->IsVulkanAvailable(), renderer->IsOpenglAvailable());

        // Loads the gltf files that were specified as command line arguments
        if(argc != 1)
        {
            for(uint32_t i = 1; i < argc; ++i)
            {
                LoadGltfScene(pResources.Data(), argv[i], renderer->IsVulkanAvailable(), renderer->IsOpenglAvailable());
            }
        }

        // Set the draw count to the render object count   
        drawCount = pResources.Data()->renderObjectCount;

        // Pass the resources and pointers to any of the renderers that might be used for rendering
        if(!renderer->SetupRequestedRenderersForDrawing(pResources.Data(), drawCount, mainCamera))
            BLIT_FATAL("Renderer failed to setup, Blitzen's rendering system is offline")
        
        // Starts the clock
        m_clockStartTime = BlitzenPlatform::PlatformGetAbsoluteTime();
        m_clockElapsedTime = 0;
        double previousTime = m_clockElapsedTime;// Initialize previous frame time to the elapsed time

        // Main Loop starts
        while(bRunning)
        {
            // Captures events
            if(!BlitzenPlatform::PlatformPumpMessages())
            {
                bRunning = 0;
            }

            if(!bSuspended)
            {
                // Get the elapsed time of the application
                m_clockElapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - m_clockStartTime;
                // Update the delta time by using the previous elapsed time
                m_deltaTime = m_clockElapsedTime - previousTime;
                // Update the previous elapsed time to the current elapsed time
                previousTime = m_clockElapsedTime;

                // With delta time retrieved, call update camera to make any necessary changes to the scene based on its transform
                UpdateCamera(mainCamera, (float)m_deltaTime);

                // Draw the frame!!!!
                renderer->DrawFrame(mainCamera, drawCount);

                // Make sure that the window resize is set to false after the renderer is notified
                mainCamera.transformData.windowResize = 0;

                BlitzenCore::UpdateInput(m_deltaTime);
            }
        }

        // Shutdown the renderers before the engine is shutdown
        renderer->ShutdownRenderers();

        // Shutdown the last few systems
        BLIT_WARN("Blitzen is shutting down")

        BlitzenCore::ShutdownLogging();

        BlitzenPlatform::PlatformShutdown();

        s_pEngine = nullptr;
    }
}







int main(int argc, char* argv[])
{
    BlitzenEngine::Engine engine;
    engine.Run(argc, argv);
}
//Assets/Scenes/CityLow/scene.gltf ../../GltfTestScenes/Scenes/Plaza/scene.gltf ../../GltfTestScenes/Scenes/Museum/scene.gltf (personal test scenes for copy+paste)