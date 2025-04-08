#pragma once 

#include "Core/blitLogger.h"
#include "Core/blitInput.h"

namespace BlitzenPlatform
{
    size_t GetPlatformMemoryRequirements();

    uint8_t PlatformStartup(const char* appName, 
        BlitzenCore::EventSystemState* pEvents, 
        BlitzenCore::InputSystemState* pInput
    );

    // Called when engine shuts down to shutdown the platform system
    void PlatformShutdown();

    uint8_t PlatformPumpMessages();

    // This is basically like glfwGetTime()
    double PlatformGetAbsoluteTime();

    void PlatformSleep(uint64_t ms);

    void* GetWindowHandle();
}