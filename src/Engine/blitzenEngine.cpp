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

            // This always returns 1 at the moment (I don't know if this would ever fail in the future to be honest)
            BLIT_ASSERT_MESSAGE(BlitzenEngine::LoadResourceSystem(m_resources), "Failed to acquire resourece system")
            

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
        // Checks if at least one of the rendering APIs was initialized
        uint8_t hasRenderer = 0;

        // Create the vulkan renderer if it's requested
        #if BLITZEN_VULKAN
            BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer> pVulkan;
            hasRenderer = CreateVulkanRenderer(pVulkan, m_platformData.windowWidth, m_platformData.windowHeight);
        #endif

        // Directx12 is here for testing purposes, there's no directx12 backend
        #if BLITZEN_DIRECTX12
            m_systems.directx12 = 1;
            hasRenderer = m_systems.directx12;
        #endif

        // Test if any renderer was initialized
        BLIT_ASSERT_MESSAGE(hasRenderer, "Blitzen cannot continue without a renderer")

        ActiveRenderer activeRenderer = BLIT_ACTIVE_RENDERER_ON_BOOT;

        BLIT_ASSERT(CheckActiveRenderer(activeRenderer))
        
        // If the engine passes the above assertion, then it means that it can run the main loop (unless some less fundamental stuff makes it fail)
        isRunning = 1;
        isSupended = 0;

        SetupCamera(m_mainCamera, BLITZEN_FOV, static_cast<float>(m_platformData.windowWidth), 
        static_cast<float>(m_platformData.windowHeight), BLITZEN_ZNEAR, BlitML::vec3(50.f, 0.f, 0.f));

        /*
            Load the default textures and some other textures for testing
        */
        {
            // Default texture at index 0
            uint32_t blitTexCol = glm::packUnorm4x8(glm::vec4(0.3, 0, 0.6, 1));
            uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	        uint32_t pixels[16 * 16]; 
	        for (int x = 0; x < 16; x++) 
            {
	        	for (int y = 0; y < 16; y++) 
                {
	        		pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : blitTexCol;
	        	}
	        }
            m_resources.textures[0].pTextureData = reinterpret_cast<uint8_t*>(pixels);
            m_resources.textures[0].textureHeight = 1;
            m_resources.textures[0].textureWidth = 1;
            m_resources.textures[0].textureChannels = 4;
            m_resources.textureTable.Set(BLIT_DEFAULT_TEXTURE_NAME, &(m_resources.textures[0]));

            // This is hardcoded now
            LoadTextureFromFile(m_resources, "Assets/Textures/cobblestone.png", "loaded_texture", pVulkan.Data(), nullptr);
            LoadTextureFromFile(m_resources, "Assets/Textures/texture.jpg", "loaded_texture2", pVulkan.Data(), nullptr);
            LoadTextureFromFile(m_resources, "Assets/Textures/cobblestone_SPEC.jpg", "spec_texture", pVulkan.Data(), nullptr);
        }

        /*
            Load the default material and some material for testing
        */
        {
            // Manually load a default material at index 0
            m_resources.materials[0].diffuseColor = BlitML::vec4(1.f);
            m_resources.materials[0].diffuseTextureTag = m_resources.textureTable.Get(BLIT_DEFAULT_TEXTURE_NAME, &m_resources.textures[0])->textureTag;
            m_resources.materials[0].specularTextureTag = m_resources.textureTable.Get(BLIT_DEFAULT_TEXTURE_NAME, &m_resources.textures[0])->textureTag;
            m_resources.materials[0].materialId = 0;
            m_resources.materialTable.Set(BLIT_DEFAULT_MATERIAL_NAME, &(m_resources.materials[0]));

            // Test code
            BlitML::vec4 color1(0.1f);
            BlitML::vec4 color2(0.2f);
            DefineMaterial(m_resources, color1, 65.f, "loaded_texture", "spec_texture", "loaded_material");
            DefineMaterial(m_resources, color2, 65.f, "loaded_texture2", "unknown", "loaded_material2");
        }

        // Load test data to draw
        LoadDefaultData(m_resources);

        // Load some hardcoded game objects to test the rendering
        uint32_t drawCount = BLITZEN_VULKAN_MAX_DRAW_CALLS / 2 + 1;// Rendering a large amount of objects to stress test the renderer
        CreateTestGameObjects(m_resources, drawCount);

        // Pass the resources and pointers to any of the renderers that might be used for rendering
        SetupRequestedRenderersForDrawing(m_resources, drawCount, pVulkan.Data());
        
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
                pVulkan.Data(), activeRenderer);

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
            m_systems.vulkan = 0;
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

    // Blitzen engine Allocated and freed inside this scope
    {
        // Blitzen engine allocated on the heap, it will cause stack overflow otherwise
        BlitCL::SmartPointer<BlitzenEngine::Engine, BlitzenCore::AllocationType::Engine> Blitzen;

        Blitzen.Data()->Run();
    }

    BlitzenCore::MemoryManagementShutdown();
}
