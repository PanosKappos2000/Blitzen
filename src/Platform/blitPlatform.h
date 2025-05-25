#pragma once 
#include "Core/blitzenEngine.h"

namespace BlitzenPlatform
{
    bool PlatformStartup(const char* appName, void* pPlatform, void* pEvents, void* pRenderer);

    bool DispatchEvents(void* pPlatform);

    void BlitzenSleep(uint64_t ms);

    void* PlatformMalloc(size_t size, uint8_t aligned);

    void PlatformFree(void* pBlock, uint8_t aligned);

    void* PlatformMemZero(void* pBlock, size_t size);

    void* PlatformMemCopy(void* pDst, void* pSrc, size_t size);

    void* PlatformMemSet(void* pDst, int32_t value, size_t size);
}