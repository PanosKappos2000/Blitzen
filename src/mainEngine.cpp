#include "mainEngine.h"
#include "Platform/platform.h"

namespace BlitzenEngine
{
    BlitzenEngine::BlitzenEngine()
    {
        WARNING_MESSAGE("Blitzen Engine Booting")
        BLIT_ASSERT(BlitzenPlatform::PlatformStartup(&m_platformState, "Blitzen Engine 1.0.C", 100, 100, 1280, 720))
    }

    void BlitzenEngine::Run()
    {
        while(true)
        {
            BlitzenPlatform::PlatformPumpMessages(&m_platformState);
        }
    }

    BlitzenEngine::~BlitzenEngine()
    {
        BlitzenPlatform::PlatformShutdown(&m_platformState);
    }
}

int main()
{
    
    BlitzenEngine::BlitzenEngine blitzen;

    blitzen.Run();

    BLIT_ASSERT_DEBUG(0)
}
