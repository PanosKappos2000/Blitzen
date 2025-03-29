#pragma once 

#include "Core/blitLogger.h"

namespace BlitzenPlatform
{
    // I am not sure if I need this
    size_t GetPlatformMemoryRequirements();

    // Called when the engine start to get a window and other related systems that are platform specific
    uint8_t PlatformStartup(const char* appName);

    // Called when engine shuts down to shutdown the platform system
    void PlatformShutdown();

    uint8_t PlatformPumpMessages();
    
    void PlatformConsoleWrite(const char* message, uint8_t color);
    void PlatformConsoleError(const char* message, uint8_t color);

    // This is basically like glfwGetTime()
    double PlatformGetAbsoluteTime();

    void PlatformSleep(uint64_t ms);

    void* GetWindowHandle();
}