#pragma once 
#include "Core/blitzenEngine.h"

namespace BlitzenPlatform
{
    bool PlatformStartup(const char* appName, void* pPlatform, void* pEvents, void* pRenderer);

    bool DispatchEvents(void* pPlatform);

    void BlitzenSleep(uint64_t ms);
}