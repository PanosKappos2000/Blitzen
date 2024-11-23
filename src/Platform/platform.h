#pragma once 

#include "Core/blitLogger.h"

namespace BlitzenPlatform
{
    struct PlatformState
    {
        //This will be cast to the instance of each platform
        void* internalState;
    };

    uint8_t PlatformStartup(PlatformState* pState, const char* appName, int32_t x, int32_t y, uint32_t width, uint32_t height);
    void PlatformShutdown(PlatformState* pState);

    uint8_t PlatformPumpMessages(PlatformState* pState);

    void* BlitMalloc(uint64_t size, uint8_t aligned);
    void BlitFree(void* pBlock, uint8_t aligned);
    void* BlitMemZero(void* pBlock, uint64_t size);
    void* BlitMemCopy(void* pDst, void* pSrc, uint64_t size);
    void* BlitMemSet(void* pDst, int32_t value, uint64_t size);

    void PlatformConsoleWrite(const char* message, uint8_t color);
    void PlatformConsoleError(const char* message, uint8_t color);

    //This is basically like glfwGetTime()
    double PlatformGetAbsoluteTime();

    void PlatformSleep(uint64_t ms);
}