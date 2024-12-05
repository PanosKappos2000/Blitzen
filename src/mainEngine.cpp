#include "mainEngine.h"
#include "Platform/platform.h"
#include "BlitzenVulkan/vulkanRenderer.h"



namespace BlitzenEngine
{
    /*-----------------------------------------------------------------------------------------
        Since having the huge renderer up here as a static variable might cause problems, 
        I will create it ouside of the engine and the engine will access it through a pointer
        given to this static variable
    ---------------------------------------------------------------------------------------------*/
    struct BlitzenRenderers
    {
        BlitzenVulkan::VulkanRenderer* pVulkan = nullptr;
    };

    static BlitzenRenderers s_renderers;

    // Static member variable needs to be declared
    Engine* Engine::pEngineInstance;

    Engine::Engine()
    {
        // There should not be a 2nd instance of Blitzen Engine
        if(GetEngineInstancePointer())
        {
            BLIT_ERROR("Blitzen is already active")
            return;
        }

        // Initialize the engine if no other instance seems to exist
        else
        {
            // The instance is the first thing that gets initalized
            pEngineInstance = this;
            m_systems.engine = 1;
            BLIT_INFO("%s Booting", BLITZEN_VERSION)

            if(BlitzenCore::InitLogging())
                BLIT_DBLOG("Test Logging")
            else
                BLIT_ERROR("Logging is not active")

            // Engine owns the event system
            if(BlitzenCore::EventsInit())
            {
                BLIT_INFO("Event system active")
                BlitzenCore::InputInit(&m_systems.inputState);
            }
            else
                BLIT_FATAL("Event system initialization failed!")

            if(!BlitzenEngine::LoadResourceSystem(m_resources.textures))
            {
                BLIT_FATAL("Resource system initalization failed")
            }

            // Assert if platform specific code is initialized, the engine cannot continue without it
            BLIT_ASSERT(BlitzenPlatform::PlatformStartup(&m_platformState, BLITZEN_VERSION, BLITZEN_WINDOW_STARTING_X, 
            BLITZEN_WINDOW_STARTING_Y, m_platformData.windowWidth, m_platformData.windowHeight))

            // Register some default events, like window closing on escape
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, OnEvent);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyPressed, nullptr, OnKeyPress);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyReleased, nullptr, OnKeyPress);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::WindowResize, nullptr, OnResize);

            BLIT_ASSERT_MESSAGE(RendererInit(), "Blitzen cannot continue without a renderer")

            isRunning = 1;
            isSupended = 0;
        }
    }

    uint8_t Engine::RendererInit()
    {
        uint8_t hasRenderer = 0;

        #if BLITZEN_VULKAN
            if(s_renderers.pVulkan)
            {
                s_renderers.pVulkan->Init(&m_platformState, m_platformData.windowWidth, m_platformData.windowHeight);
                m_systems.vulkan = 1;
                hasRenderer = 1;
                m_renderer = ActiveRenderer::Vulkan;
            }
        #endif

        return hasRenderer;
    }



    void Engine::Run()
    {
        m_camera.projectionMatrix = BlitML::Perspective(BlitML::Radians(45.f), static_cast<float>(m_platformData.windowWidth) / 
        static_cast<float>(m_platformData.windowHeight), 10000.f, 0.1f);
        m_camera.viewMatrix = BlitML::Mat4Inverse(BlitML::Translate(BlitML::vec3(0.f, 0.f, 5.f)));
        m_camera.projectionViewMatrix = m_camera.projectionMatrix * m_camera.viewMatrix;

        // Loads textures that were requested
        LoadTextures();

        // Before starting the clock, the engine will put its renderer on the ready state
        SetupForRenderering();

        // Should be called right before the main loop starts
        StartClock();
        double previousTime = m_clock.elapsedTime;
        // Main Loop starts
        while(isRunning)
        {
            if(!BlitzenPlatform::PlatformPumpMessages(&m_platformState))
            {
                isRunning = 0;
            }

            if(!isSupended)
            {
                // Update the clock and deltaTime
                m_clock.elapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - m_clock.startTime;
                double deltaTime = m_clock.elapsedTime - previousTime;
                previousTime = m_clock.elapsedTime;

                UpdateCamera(m_camera, (float)deltaTime);

                DrawFrame();

                // Make sure that the window resize is set to false after the renderer is notified
                m_platformData.windowResize = 0;

                BlitzenCore::UpdateInput(deltaTime);
            }
        }
        // Main loop ends
        StopClock();
    }

    void Engine::LoadTextures()
    {
        // Default texture at index 0
        uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	    uint32_t pixels[16 * 16]; 
	    for (int x = 0; x < 16; x++) 
        {
	    	for (int y = 0; y < 16; y++) 
            {
	    		pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : white;
	    	}
	    }
        m_resources.textures[0].pTextureData = reinterpret_cast<uint8_t*>(pixels);
        m_resources.textures[0].textureHeight = 1;
        m_resources.textures[0].textureWidth = 1;
        m_resources.textures[0].textureChannels = 4;
        m_resources.textureTable.Set(BLIT_DEFAULT_TEXTURE_NAME, &(m_resources.textures[0]));
        

        if(LoadTextureFromFile("Assets/cobblestone.png"))
            // The last texture loaded should always be added to the texture hashmap (temporary code, there will be an actual system that automatically does this)
            m_resources.textureTable.Set("Loaded texture", &(m_resources.textures[GetTotalLoadedTexturesCount() - 1]));
    }

    void Engine::SetupForRenderering()
    {
        #if BLITZEN_VULKAN
            // Temporary shader data for tests on vulkan rendering
            BlitCL::DynamicArray<BlitML::Vertex> vertices(8);
            BlitCL::DynamicArray<uint32_t> indices(32);
            vertices[0].position = BlitML::vec3(-0.5f, 0.5f, 0.f);
            vertices[0].uvX = 0.f; 
            vertices[0].uvY = 0.f;
            vertices[1].position = BlitML::vec3(-0.5f, -0.5f, 0.f);
            vertices[1].uvX = 0.f; 
            vertices[1].uvY = 1.f;
            vertices[2].position = BlitML::vec3(0.5f, -0.5f, 0.f);
            vertices[2].uvX = 1.f;
            vertices[2].uvY = 1.f;
            vertices[3].position = BlitML::vec3(0.5f, 0.5f, 0.f);
            vertices[3].uvX = 1.f;
            vertices[3].uvY = 0.f;
            vertices[4].position = BlitML::vec3(-0.5f, 0.5f, -0.5f);
            
            vertices[5].position = BlitML::vec3(-0.5f, -0.5f, -0.5f);
            
            vertices[6].position = BlitML::vec3(0.5f, -0.5f, -0.5f);
            
            vertices[7].position = BlitML::vec3(0.5f, 0.5f, -0.5f);
            
            indices[0] = 0;
            indices[1] = 1;
            indices[2] = 2;
            indices[3] = 2;
            indices[4] = 3;
            indices[5] = 0;
            indices[6] = 4;
            indices[7] = 5;
            indices[8] = 6;
            indices[9] = 6;
            indices[10] = 7;
            indices[11] = 4;
            indices[12] = 4;
            indices[13] = 5;
            indices[14] = 1;
            indices[15] = 1;
            indices[16] = 0;
            indices[17] = 4;
            indices[18] = 7;
            indices[19] = 6;
            indices[20] = 2;
            indices[21] = 2;
            indices[22] = 3;
            indices[23] = 7;
            BlitCL::DynamicArray<BlitzenVulkan::StaticRenderObject> renders(1);
            renders[0].modelMatrix = BlitML::Translate(BlitML::vec3(0.f, 0.f, 4.f));
            renders[0].textureTag = 1;

            s_renderers.pVulkan->UploadDataToGPUAndSetupForRendering(vertices, indices, renders, m_resources.textures, GetTotalLoadedTexturesCount());
        #endif
    }

    void Engine::StartClock()
    {
        m_clock.startTime = BlitzenPlatform::PlatformGetAbsoluteTime();
        m_clock.elapsedTime = 0;
    }

    void Engine::StopClock()
    {
        m_clock.elapsedTime = 0;
    }

    // This will move from here once I add a camera system
    void UpdateCamera(Camera& camera, float deltaTime)
    {
        if (camera.cameraDirty)
        {
            // I haven't overloaded the += operator
            camera.position = camera.position + camera.velocity * deltaTime * 95.f; // Needs this 95.f stabilizer, otherwise deltaTime teleports it

            BlitML::mat4 translation = BlitML::Mat4Inverse(BlitML::Translate(camera.position));

            camera.viewMatrix = translation; // Normally, I would also add rotation here but the math library has a few problems at the moment
            camera.projectionViewMatrix = camera.projectionMatrix * camera.viewMatrix;
        }
    }

    void Engine::DrawFrame()
    {
        switch(m_renderer)
        {
            #if BLITZEN_VULKAN
            case ActiveRenderer::Vulkan:
            {
                if(m_systems.vulkan)
                {
                    BlitzenVulkan::RenderContext renderContext;
                    renderContext.windowResize = m_platformData.windowResize;
                    renderContext.windowWidth = m_platformData.windowWidth;
                    renderContext.windowHeight = m_platformData.windowHeight;
                    renderContext.projectionMatrix = m_camera.projectionMatrix;
                    renderContext.viewMatrix = m_camera.viewMatrix;
                    renderContext.projectionView = m_camera.projectionViewMatrix;

                    s_renderers.pVulkan->DrawFrame(renderContext);
                }
                break;
            }
            #endif
            default:
            {
                break;
            }
        }
    }



    Engine::~Engine()
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

            m_renderer = ActiveRenderer::MaxRenderers;
            RendererShutdown();

            BlitzenPlatform::PlatformShutdown(&m_platformState);

            m_systems.engine = 0;
            pEngineInstance = nullptr;
        }

        // The destructor should not be calle more than once as it will crush the application
        else
        {
            BLIT_ERROR("Any uninitialized instances of Blitzen will not be explicitly cleaned up")
        }
    }

    void Engine::RendererShutdown()
    {
        #if BLITZEN_VULKAN
            m_systems.vulkan = 0;
            s_renderers.pVulkan->Shutdown();
        #endif
    }




    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        if(eventType == BlitzenCore::BlitEventType::EngineShutdown)
        {
            BLIT_WARN("Engine shutdown event encountered!")
            BlitzenEngine::Engine::GetEngineInstancePointer()->RequestShutdown();
            return 1; 
        }

        return 0;
    }

    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        //Get the key pressed from the event context
        BlitzenCore::BlitKey key = static_cast<BlitzenCore::BlitKey>(data.data.ui16[0]);

        if(eventType == BlitzenCore::BlitEventType::KeyPressed)
        {
            switch(key)
            {
                case BlitzenCore::BlitKey::__ESCAPE:
                {
                    BlitzenCore::EventContext newContext = {};
                    BlitzenCore::FireEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, newContext);
                    return 1;
                }
                case BlitzenCore::BlitKey::__W:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.cameraDirty = 1;
                    camera.velocity = BlitML::vec3(0.f, 0.f, -1.f);
                    break;
                }
                case BlitzenCore::BlitKey::__S:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.cameraDirty = 1;
                    camera.velocity = BlitML::vec3(0.f, 0.f, 1.f);
                    break;
                }
                case BlitzenCore::BlitKey::__A:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.cameraDirty = 1;
                    camera.velocity = BlitML::vec3(-1.f, 0.f, 0.f);
                    break;
                }
                case BlitzenCore::BlitKey::__D:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.cameraDirty = 1;
                    camera.velocity = BlitML::vec3(1.f, 0.f, 0.f);
                    break;
                }
                default:
                {
                    BLIT_DBLOG("Key pressed %i", key)
                    return 1;
                }
            }
        }
        else if (eventType == BlitzenCore::BlitEventType::KeyReleased)
        {
            switch (key)
            {
                case BlitzenCore::BlitKey::__W:
                case BlitzenCore::BlitKey::__S:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.velocity.z = 0.f;
                    if(camera.velocity.y == 0.f && camera.velocity.x == 0.f)
                    {
                        camera.cameraDirty = 0;
                    }
                    break;
                }
                case BlitzenCore::BlitKey::__A:
                case BlitzenCore::BlitKey::__D:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.velocity.x = 0.f;
                    if (camera.velocity.y == 0.f && camera.velocity.z == 0.f)
                    {
                        camera.cameraDirty = 0;
                    }
                    break;
                }
            }
        }
        return 0;
    }

    uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        uint32_t newWidth = data.data.ui32[0];
        uint32_t newHeight = data.data.ui32[1];

        Engine::GetEngineInstancePointer()->UpdateWindowSize(newWidth, newHeight);

        return 1;
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

        m_camera.projectionMatrix = BlitML::Perspective(BlitML::Radians(45.f), (float)newWidth / (float)newHeight,
        10000.f, 0.1f);
        m_camera.projectionViewMatrix = m_camera.projectionMatrix * m_camera.viewMatrix;
    }
}







int main()
{
    BlitzenCore::MemoryManagementInit();

    // Blitzen needs to be destroyed before memory management can shutdown, otherwise the memory system will complain
    {
        #if BLITZEN_VULKAN
            BlitzenVulkan::VulkanRenderer vulkan;
            BlitzenEngine::s_renderers.pVulkan = &vulkan;
        #endif

        BlitzenEngine::Engine blitzen;

        blitzen.Run();
    }

    BlitzenCore::MemoryManagementShutdown();
}
