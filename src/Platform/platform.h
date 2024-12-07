#pragma once 

#include "Core/blitLogger.h"


namespace BlitzenPlatform
{
    size_t GetPlatformMemoryRequirements();
    uint8_t PlatformStartup(void* pState, const char* appName, int32_t x, int32_t y, uint32_t width, uint32_t height);
    void PlatformShutdown();

    uint8_t PlatformPumpMessages();

    /* -------------------------------------------------------------------------------------------------------
        These will not be called by systems directly, they're meant to aid the custom allocation functions, 
        since some memory functionality might be platform specific
    --------------------------------------------------------------------------------------------------------  */
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