#pragma once 

#include "Core/blitLogger.h"
#include "Core/blitInput.h"

namespace BlitzenPlatform
{
    size_t GetPlatformMemoryRequirements();

    bool PlatformStartup(const char* appName, BlitzenCore::EventSystemState* pEvents, 
        BlitzenCore::InputSystemState* pInput);

    // Called when engine shuts down to shutdown the platform system
    void PlatformShutdown();

    bool PlatformPumpMessages();

    void PlatformSleep(uint64_t ms);

    void* GetWindowHandle();
}