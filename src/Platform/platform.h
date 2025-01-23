#pragma once 

#include "Core/blitLogger.h"

namespace BlitzenPlatform
{
    // I am not sure if I need this
    size_t GetPlatformMemoryRequirements();

    // Called when the engine start to get a window and other related systems that are platform specific
    uint8_t PlatformStartup(const char* appName, int32_t x, int32_t y, uint32_t width, uint32_t height);

    // Called when engine shuts down to shutdown the platform system
    void PlatformShutdown();

    uint8_t PlatformPumpMessages();

    // Called only by the memory manager
    void* PlatformMalloc(size_t size, uint8_t aligned);
    void PlatformFree(void* pBlock, uint8_t aligned);
    void* PlatformMemZero(void* pBlock, size_t size);
    void* PlatformMemCopy(void* pDst, void* pSrc, size_t size);
    void* PlatformMemSet(void* pDst, int32_t value, size_t size);

    void PlatformConsoleWrite(const char* message, uint8_t color);
    void PlatformConsoleError(const char* message, uint8_t color);

    // This is basically like glfwGetTime()
    double PlatformGetAbsoluteTime();

    void PlatformSleep(uint64_t ms);
}