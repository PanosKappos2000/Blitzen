#pragma once 
#include "Core/blitzenEngine.h"

namespace BlitzenPlatform
{
    size_t GetPlatformMemoryRequirements();

    // Makes call to the OS for window creation and initializes the rendering API
    bool PlatformStartup(const char* appName, void* pEvents, void* pRenderer);

    // Called when engine shuts down to shutdown the platform system
    void PlatformShutdown();

    bool PlatformPumpMessages();

    void PlatformSleep(uint64_t ms);

    void* GetWindowHandle();
}