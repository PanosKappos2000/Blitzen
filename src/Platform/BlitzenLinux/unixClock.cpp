#if defined(linux)

#include "Platform/blitPlatformContext.h"
#include "Core/Events/blitTimeManager.h"

namespace BlitzenPlatform
{
    void PlatfrormSetupClock(BlitzenCore::WorldTimeManager* pClock)
    {
        
    }

    double PlatformGetAbsoluteTime(double frequence) 
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        // I don't know if this is any good I did not write it
        return now.tv_sec + now.tv_nsec * 0.000000001;
    }
}

#endif