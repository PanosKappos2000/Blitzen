#if defined(_WIN32)

#include "Platform/blitPlatformContext.h"
#include "Core/Events/blitTimeManager.h"

namespace BlitzenPlatform
{
    void PlatfrormSetupClock(BlitzenCore::WorldTimeManager* pClock)
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        pClock->mClockFrequency = 1.0 / double(frequency.QuadPart);
        //QueryPerformanceCounter(&inl_startTime); LARGE_INTEGER startTime never used
    }

    double PlatformGetAbsoluteTime(double clockFrequency)
    {
        LARGE_INTEGER nowTime;
        QueryPerformanceCounter(&nowTime);
        return double(nowTime.QuadPart) * clockFrequency;
    }
}

#endif