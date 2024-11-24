#include "mainEngine.h"
#include "Platform/platform.h"

namespace BlitzenEngine
{
    // Static member variable needs to be declared
    Engine* Engine::pEngineInstance;

    Engine::Engine()
    {
        if(GetEngineInstancePointer())
        {
            ERROR_MESSAGE("Blitzen is already active")
        }
        else
        {
            pEngineInstance = this;

            WARNING_MESSAGE("Blitzen Engine Booting")

            BLIT_ASSERT(BlitzenPlatform::PlatformStartup(&m_platformState, BLITZEN_VERSION, BLITZEN_WINDOW_STARTING_X, 
            BLITZEN_WINDOW_STARTING_Y, m_platformData.windowWidth, m_platformData.windowHeight))

            BLIT_ASSERT(BlitzenCore::InitLogging())

            isRunning = 1;
            isSupended = 0;
        }
    }

    void Engine::Run()
    {
        while(isRunning)
        {
            BlitzenPlatform::PlatformPumpMessages(&m_platformState);
        }
    }

    Engine::~Engine()
    {
        BlitzenPlatform::PlatformShutdown(&m_platformState);
    }
}

int main()
{
    BlitzenEngine::Engine blitzen;

    blitzen.Run();
}
