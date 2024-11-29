#include "mainEngine.h"
#include "Platform/platform.h"

namespace BlitzenEngine
{
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
                BlitzenCore::InputInit();
            }
            else
                BLIT_FATAL("Event system initialization failed!")

            // Assert if platform specific code is initialized, the engine cannot continue without it
            BLIT_ASSERT(BlitzenPlatform::PlatformStartup(&m_platformState, BLITZEN_VERSION, BLITZEN_WINDOW_STARTING_X, 
            BLITZEN_WINDOW_STARTING_Y, m_platformData.windowWidth, m_platformData.windowHeight))

            // Register some default events, like window closing on escape
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, OnEvent);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyPressed, nullptr, OnKeyPress);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyReleased, nullptr, OnKeyPress);

            BLIT_ASSERT_MESSAGE(RendererInit(), "Blitzen cannot continue without a renderer")

            isRunning = 1;
            isSupended = 0;
        }
    }

    uint8_t Engine::RendererInit()
    {
        uint8_t hasRenderer = 0;

        #if BLITZEN_VULKAN
            m_systems.vulkanRenderer.Init(&m_platformState, m_platformData.windowWidth, m_platformData.windowHeight);
            m_systems.vulkan = 1;
            hasRenderer = 1;
            m_renderer = ActiveRenderer::Vulkan;
        #endif

        return hasRenderer;
    }



    void Engine::Run()
    {
        // Before starting the clock, the engine will put its renderer on the ready state
        SetupForRenderering();

        // Should be called right before the main loop starts
        StartClock();
        double previousTime = m_clock.elapsedTime;
        // Main Loop starts
        while(isRunning)
        {
            // This does nothing for now
            BlitzenPlatform::PlatformPumpMessages(&m_platformState);

            if(!isSupended)
            {
                // Update the clock and deltaTime
                m_clock.elapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - m_clock.startTime;
                double deltaTime = m_clock.elapsedTime - previousTime;
                previousTime = m_clock.elapsedTime;

                DrawFrame();

                BlitzenCore::UpdateInput(deltaTime);
            }
        }
        // Main loop ends
        StopClock();
    }

    void Engine::SetupForRenderering()
    {
        #if BLITZEN_VULKAN
            m_systems.vulkanRenderer.UploadDataToGPUAndSetupForRendering();
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

    void Engine::DrawFrame()
    {
        switch(m_renderer)
        {
            case ActiveRenderer::Vulkan:
            {
                if(m_systems.vulkan)
                    m_systems.vulkanRenderer.DrawFrame(nullptr);
                break;
            }
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
            m_systems.vulkanRenderer.Shutdown();
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
                default:
                {
                    BLIT_DBLOG("Key pressed %i", key)
                    return 1;
                }
            }
        }
        return 0;
    }
}







int main()
{
    BlitzenCore::MemoryManagementInit();

    // Blitzen needs to be destroyed before memory management can shutdown, otherwise the memory system will complain
    {
        BlitzenEngine::Engine blitzen;

        // This should not work and will cause an error message
        BlitzenEngine::Engine shouldNotBeAllowed;

        blitzen.Run();
    }

    BlitzenCore::MemoryManagementShutdown();
}
