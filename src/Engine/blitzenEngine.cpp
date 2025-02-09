/* 
    The code in this file calls all of the functions needed to run the applications.
    Contains the main function at the bottom
*/
 
#include "Engine/blitzenEngine.h"
#include "Platform/platform.h"
#include "Renderer/blitRenderer.h"
#include "Core/blitzenCore.h"
#include "Game/blitCamera.h"

#define BLIT_ACTIVE_RENDERER_ON_BOOT      BlitzenEngine::ActiveRenderer::Opengl

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
            BLIT_INFO("%s Booting", BLITZEN_VERSION)
        }
    }



    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        This function currently holds the majority of the functionality called during runtime.
        Its scope owns the memory used by each renderer(only Vulkan for the forseeable future.
        It calls every function needed to draw a frame and other functionality the engine has
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    void Engine::Run(uint32_t argc, char* argv[])
    {
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
        BLIT_ASSERT(BlitzenPlatform::PlatformStartup(BLITZEN_VERSION, BLITZEN_WINDOW_STARTING_X, 
        BLITZEN_WINDOW_STARTING_Y, BLITZEN_WINDOW_WIDTH, BLITZEN_WINDOW_HEIGHT))
            
        // With the event and input systems active, register the engine's default events and input bindings
        RegisterDefaultEvents();

        RenderingSystem renderer;
        
        // Allocated the rendering resources on the heap, it is too big for the stack of this function
        BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer> pResources;
        BLIT_ASSERT_MESSAGE(BlitzenEngine::LoadRenderingResourceSystem(pResources.Data()), "Failed to acquire resource system")

        // Checks if at least one of the rendering APIs was initialized
        uint8_t hasRenderer = 0;

        // Create the vulkan renderer if it's requested
        #if BLITZEN_VULKAN
            BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer> pVulkan;
            hasRenderer = CreateVulkanRenderer(pVulkan, BLITZEN_WINDOW_WIDTH, BLITZEN_WINDOW_HEIGHT) || hasRenderer;
        #endif

        #if BLITZEN_OPENGL
            BlitzenGL::OpenglRenderer gl;
            hasRenderer = CreateOpenglRenderer(gl, BLITZEN_WINDOW_WIDTH, BLITZEN_WINDOW_HEIGHT) || hasRenderer;
        #endif

        // Test if any renderer was initialized
        BLIT_ASSERT_MESSAGE(hasRenderer, "Blitzen cannot continue without a renderer")

        // Initialize the active renderer and assert if it is available
        BLIT_ASSERT(SetActiveRenderer(BLIT_ACTIVE_RENDERER_ON_BOOT))
        
        // If the engine passes the above assertion, then it means that it can run the main loop (unless some less fundamental stuff makes it fail)
        isRunning = 1;
        isSupended = 0;

        // Setup the main camera
        Camera& mainCamera = cameraSystem.GetCamera();
        SetupCamera(mainCamera, BLITZEN_FOV, static_cast<float>(BLITZEN_WINDOW_WIDTH), 
        static_cast<float>(BLITZEN_WINDOW_HEIGHT), BLITZEN_ZNEAR, BlitML::vec3(30.f, 100.f, 0.f), BLITZEN_DRAW_DISTANCE);

        // Loads obj meshes that will be draw with "random" transforms by the millions to stress the renderer
        uint32_t drawCount = BLITZEN_MAX_DRAW_OBJECTS / 2 + 1;// Rendering a large amount of objects to stress test the renderer
        LoadGeometryStressTest(pResources.Data(), drawCount, pVulkan.Data(), nullptr);

        // Loads the gltf files that were specified as command line arguments
        if(argc == 1)
            LoadGltfScene(pResources.Data(), "Assets/Scenes/CityLow/scene.gltf", 1, pVulkan.Data(), &gl);
        else
        {
            for(uint32_t i = 1; i < argc; ++i)
            {
                LoadGltfScene(pResources.Data(), argv[i], 1, pVulkan.Data(), &gl);
            }
        }

        // Set the draw count to the render object count   
        drawCount = pResources.Data()->renderObjectCount;

        // Pass the resources and pointers to any of the renderers that might be used for rendering
        BLIT_ASSERT(SetupRequestedRenderersForDrawing(pResources.Data(), drawCount, mainCamera));/* I use an assertion here
        but it could be handled some other way as well */
        
        // Start the clock
        m_clockStartTime = BlitzenPlatform::PlatformGetAbsoluteTime();
        m_clockElapsedTime = 0;
        double previousTime = m_clockElapsedTime;// Initialize previous frame time to the elapsed time

        // Main Loop starts
        while(isRunning)
        {
            // Always returns 1 (not sure if I want messages to stop the application ever)
            if(!BlitzenPlatform::PlatformPumpMessages())
            {
                isRunning = 0;
            }

            if(!isSupended)
            {
                // Get the elapsed time of the application
                m_clockElapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - m_clockStartTime;
                // Update the delta time by using the previous elapsed time
                m_deltaTime = m_clockElapsedTime - previousTime;
                // Update the previous elapsed time to the current elapsed time
                previousTime = m_clockElapsedTime;

                // Get the moving camera pointer. If certain debug modes are active, it will differ from the main camera
                Camera* pMovingCamera = cameraSystem.GetMovingCamera();

                // With delta time retrieved, call update camera to make any necessary changes to the scene based on its transform
                UpdateCamera(*pMovingCamera, (float)m_deltaTime);

                // Draw the frame!!!!
                RenderContext renderContext(pResources.Data()->cullingData, pResources.Data()->shaderData);
                DrawFrame(mainCamera, pMovingCamera, drawCount, windowResize, renderContext);

                // Make sure that the window resize is set to false after the renderer is notified
                windowResize = 0;

                BlitzenCore::UpdateInput(m_deltaTime);
            }
        }

        // Shutdown the renderers before the engine is shutdown
        ShutdownRenderers();

        // With the main loop done, Blitzen calls Shutdown on itself
        Shutdown();
    }

    void Engine::Shutdown()
    {
        if (s_pEngine)
        {
            BLIT_WARN("Blitzen is shutting down")

            BlitzenCore::ShutdownLogging();

            BlitzenPlatform::PlatformShutdown();

            s_pEngine = nullptr;
        }

        // The destructor should not be called more than once as it will crush the application
        else
        {
            BLIT_ERROR("Any uninitialized instances of Blitzen will not be explicitly cleaned up")
        }
    }

    void Engine::UpdateWindowSize(uint32_t newWidth, uint32_t newHeight) 
    {
        
        windowResize = 1;
        if(newWidth == 0 || newHeight == 0)
        {
            isSupended = 1;
            return;
        }
        isSupended = 0;

        Camera& camera = CameraSystem::GetCameraSystem()->GetCamera();
        UpdateProjection(camera, BLITZEN_FOV, static_cast<float>(newWidth), static_cast<float>(newHeight), BLITZEN_ZNEAR);
    }
}







int main(int argc, char* argv[])
{
    // Memory management is initialized here. The engine destructor must be called before the memory manager destructor
    BlitzenCore::MemoryManagerState blitzenMemory;

    // Blitzen engine lives in this scope, it needs to go out of scope before memory management shuts down
    {
        // I could have the Engine be stack allocated, but I am keeping it as a smart pointer for now
        BlitCL::SmartPointer<BlitzenEngine::Engine, BlitzenCore::AllocationType::Engine> engine;

        engine.Data()->Run(argc, argv);
    }
}
//Assets/Scenes/CityLow/scene.gltf ../../GltfTestScenes/Scenes/Plaza/scene.gltf ../../GltfTestScenes/Scenes/Museum/scene.gltf (personal test scenes for copy+paste)