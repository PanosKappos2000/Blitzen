#include "mainEngine.h"
#include "Platform/platform.h"

namespace BlitzenEngine
{
    // Static member variable needs to be declared
    Engine* Engine::pEngineInstance;

    Engine::Engine(BlitzenCore::MemoryManager& manager)
        :m_blitzenMemory(manager)
    {
        // There should not be a 2nd instance of Blitzen Engine
        if(GetEngineInstancePointer())
        {
            ERROR_MESSAGE("Blitzen is already active")
            return;
        }
        else
        {
            pEngineInstance = this;

            if(BlitzenCore::InitLogging())
                DEBUG_MESSAGE("Test Logging")
            else
                ERROR_MESSAGE("Logging is not active")

            INFO_MESSAGE("%s Booting", BLITZEN_VERSION)

            BLIT_ASSERT(BlitzenPlatform::PlatformStartup(&m_platformState, BLITZEN_VERSION, BLITZEN_WINDOW_STARTING_X, 
            BLITZEN_WINDOW_STARTING_Y, m_platformData.windowWidth, m_platformData.windowHeight))

            BLIT_ASSERT(BlitzenCore::InitLogging())

            isRunning = 1;
            isSupended = 0;
        }
    }

    void Engine::Run()
    {
        m_blitzenMemory.BlitAlloc(BlitzenCore::AllocationType::Unkown, 0);
        m_blitzenMemory.BlitAlloc(BlitzenCore::AllocationType::MaxTypes, 50);

        while(isRunning)
        {
            BlitzenPlatform::PlatformPumpMessages(&m_platformState);
        }
    }

    Engine::~Engine()
    {
        BlitzenCore::ShutdownLogging();

        BlitzenPlatform::PlatformShutdown(&m_platformState);
    }
}

int main()
{
    // The memory manager is initialized outside of the engine, as it might need to allocate some subsystems before the engine
    BlitzenCore::MemoryManager blitMemory;

    BlitzenEngine::Engine blitzen(blitMemory);

    BlitzenEngine::Engine shouldNotBeAllowed(blitMemory);

    blitzen.Run();
}
