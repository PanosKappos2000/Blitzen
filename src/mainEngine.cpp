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
        else
        {
            // The instance is the first thing that gets initalized
            pEngineInstance = this;
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

            BLIT_ASSERT(BlitzenPlatform::PlatformStartup(&m_platformState, BLITZEN_VERSION, BLITZEN_WINDOW_STARTING_X, 
            BLITZEN_WINDOW_STARTING_Y, m_platformData.windowWidth, m_platformData.windowHeight))

            isRunning = 1;
            isSupended = 0;
        }
    }

    void Engine::Run()
    {
        while(isRunning)
        {
            // This does nothing for now
            BlitzenPlatform::PlatformPumpMessages(&m_platformState);
        }
    }

    Engine::~Engine()
    {
        BLIT_WARN("Blitzen is shutting down")

        m_systems.logSystem = 0;
        BlitzenCore::ShutdownLogging();

        m_systems.eventSystem = 0;
        BlitzenCore::EventsShutdown();

        m_systems.inputSystem = 0;
        BlitzenCore::InputShutdown();

        BlitzenPlatform::PlatformShutdown(&m_platformState);

        pEngineInstance = nullptr;
    }
}

int main()
{
    BlitzenCore::MemoryManagementInit();

    BlitzenEngine::Engine blitzen;

    // This will fail the application, so it will mostly be commented out, and will be removed later
    //BlitzenEngine::Engine shouldNotBeAllowed;

    BlitzenCore::MemoryManagementShutdown();

    blitzen.Run();

    BlitzenCore::MemoryManagementShutdown();
}
