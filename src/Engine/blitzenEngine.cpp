/* 
    The code in this file calls all of the functions needed to run the applications.
    Contains the main function at the bottom
*/
 
#include "Engine/blitzenEngine.h"
#include "Platform/platform.h"
#include "Renderer/blitRenderer.h"

#define BLIT_ACTIVE_RENDERER_ON_BOOT      BlitzenEngine::ActiveRenderer::Vulkan

namespace BlitzenEngine
{
    // This will hold pointer to all the renderers (not really necessary but whatever)
    struct BlitzenRenderers
    {
        BlitzenVulkan::VulkanRenderer* pVulkan = nullptr;
    };

    // Static member variable needs to be declared in the .cpp file as well
    Engine* Engine::pEngineInstance;

    Engine::Engine()
        :m_mainCamera{m_cameraContainer.cameraList[BLIT_MAIN_CAMERA_ID]}, // The main camera is the first element in the camera list
        m_pMovingCamera{&m_cameraContainer.cameraList[BLIT_MAIN_CAMERA_ID]} // The moving camera is a reference to the same camera as the main one initially
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
            pEngineInstance = this;
            m_systems.engine = 1;
            BLIT_INFO("%s Booting", BLITZEN_VERSION)

            BLIT_ASSERT(BlitzenCore::InitLogging())// This function does nothing at them moment and always returns 1

            if(BlitzenCore::EventsInit())
            {
                BLIT_INFO("Event system active")
                BlitzenCore::InputInit(&m_systems.inputState);// Activate the input system if the event system was succesfully created
            }
            else
                BLIT_FATAL("Event system initialization failed!")
            
            // Platform specific code initalization. Mostly window(not Windows) related things
            BLIT_ASSERT(BlitzenPlatform::PlatformStartup(BLITZEN_VERSION, BLITZEN_WINDOW_STARTING_X, 
            BLITZEN_WINDOW_STARTING_Y, m_platformData.windowWidth, m_platformData.windowHeight))

            // Register some default events, like window closing on escape and default inputs for camera and debugging
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, OnEvent);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyPressed, nullptr, OnKeyPress);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyReleased, nullptr, OnKeyPress);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::WindowResize, nullptr, OnResize);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::MouseMoved, nullptr, OnMouseMove);
        }
    }



    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        This function currently holds the majority of the functionality called during runtime.
        Its scope owns the memory used by each renderer(only Vulkan for the forseeable future.
        It calls every function needed to draw a frame and other functionality the engine has
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    void Engine::Run()
    {
        // Allocated the rendering resources on the heap, it is too big for the stack of this function
        BlitCL::SmartPointer<BlitzenEngine::RenderingResources, BlitzenCore::AllocationType::Renderer> pResources;
        BLIT_ASSERT_MESSAGE(BlitzenEngine::LoadRenderingResourceSystem(pResources.Data()), "Failed to acquire resourece system")

        // Checks if at least one of the rendering APIs was initialized
        uint8_t hasRenderer = 0;

        // Create the vulkan renderer if it's requested
        #if BLITZEN_VULKAN
            BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer> pVulkan;
            hasRenderer = CreateVulkanRenderer(pVulkan, m_platformData.windowWidth, m_platformData.windowHeight);
        #endif

        // Test if any renderer was initialized
        BLIT_ASSERT_MESSAGE(hasRenderer, "Blitzen cannot continue without a renderer")

        // Initialize the active renderer and assert if it is available
        ActiveRenderer activeRenderer = BLIT_ACTIVE_RENDERER_ON_BOOT;
        BLIT_ASSERT(CheckActiveRenderer(activeRenderer))
        
        // If the engine passes the above assertion, then it means that it can run the main loop (unless some less fundamental stuff makes it fail)
        isRunning = 1;
        isSupended = 0;

        SetupCamera(m_mainCamera, BLITZEN_FOV, static_cast<float>(m_platformData.windowWidth), 
        static_cast<float>(m_platformData.windowHeight), BLITZEN_ZNEAR, BlitML::vec3(50.f, 0.f, 0.f));
        m_mainCamera.drawDistance = BLITZEN_DRAW_DISTANCE;// Should be added to the constructor but I am kinda lazy

        
        #if BLITZEN_GEOMETRY_STRESS_TEST
            // Load some hardcoded game objects to test the rendering
            uint32_t drawCount = BLITZEN_VULKAN_MAX_DRAW_CALLS / 2 + 1;// Rendering a large amount of objects to stress test the renderer
            LoadGeometryStressTest(pResources.Data(), drawCount, pVulkan.Data(), nullptr);
        #elif BLITZEN_GLTF_SCENE
            //LoadScene(filepath)
            uint32_t drawCount = 0;
        #else
            // There are no draws if none of the valid modes are active
            uint32_t drawCount = 0;
        #endif

        // Pass the resources and pointers to any of the renderers that might be used for rendering
        SetupRequestedRenderersForDrawing(pResources.Data(), drawCount, m_mainCamera);
        
        // Start the clock
        m_clock.startTime = BlitzenPlatform::PlatformGetAbsoluteTime();
        m_clock.elapsedTime = 0;
        double previousTime = m_clock.elapsedTime;// Initialize previous frame time to the elapsed time

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
                m_clock.elapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - m_clock.startTime;
                // Update the delta time by using the previous elapsed time
                m_deltaTime = m_clock.elapsedTime - previousTime;
                // Update the previous elapsed time to the current elapsed time
                previousTime = m_clock.elapsedTime;

                // With delta time retrieved, call update camera to make any necessary changes to the scene based on its transform
                UpdateCamera(*m_pMovingCamera, (float)m_deltaTime);

                // Draw the frame
                DrawFrame(m_mainCamera, m_pMovingCamera, drawCount, 
                m_platformData.windowWidth, m_platformData.windowHeight, m_platformData.windowResize, 
                activeRenderer);

                // Make sure that the window resize is set to false after the renderer is notified
                m_platformData.windowResize = 0;

                BlitzenCore::UpdateInput(m_deltaTime);
            }
        }
        // Main loop ends

        // Zeroing the clock right now is not necessary but if there are many possible game loops, then this needs to happen
        m_clock.elapsedTime = 0;

        // The renderer is shutdown here because it will go out of scope if this run function goes out of scope
        #if BLITZEN_VULKAN
            pVulkan.Data()->Shutdown();
        #endif

        // With the main loop done, Blitzen calls Shutdown on itself
        Shutdown();
    }

    void Engine::Shutdown()
    {
        if (m_systems.engine)
        {
            BLIT_WARN("Blitzen is shutting down")

            m_systems.logSystem = 0;
            BlitzenCore::ShutdownLogging();

            m_systems.eventSystem = 0;
            BlitzenCore::EventsShutdown();

            m_systems.inputSystem = 0;
            BlitzenCore::InputShutdown();

            BlitzenPlatform::PlatformShutdown();

            m_systems.engine = 0;
            pEngineInstance = nullptr;
        }

        // The destructor should not be calle more than once as it will crush the application
        else
        {
            BLIT_ERROR("Any uninitialized instances of Blitzen will not be explicitly cleaned up")
        }
    }

    void Engine::UpdateWindowSize(uint32_t newWidth, uint32_t newHeight) 
    {
        m_platformData.windowWidth = newWidth; 
        m_platformData.windowHeight = newHeight;
        m_platformData.windowResize = 1;
        if(newWidth == 0 || newHeight == 0)
        {
            isSupended = 1;
            return;
        }
        isSupended = 0;

        UpdateProjection(m_mainCamera, BLITZEN_FOV, static_cast<float>(newWidth), static_cast<float>(newHeight), BLITZEN_ZNEAR);
    }
}







int main()
{
    BlitzenCore::MemoryManagementInit();

    // Blitzen engine leaves in this scope, it needs to go out of scope before memory management shuts down
    {
        BlitzenEngine::Engine engine;

        // This is the true main function of the application
        engine.Run();
    }

    BlitzenCore::MemoryManagementShutdown();
}
