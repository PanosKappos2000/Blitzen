#pragma once
#include "Core/blitzenEngine.h"

namespace BlitzenCore
{
    class WorldTimeManager
    {
    public:
        WorldTimeManager();
        void Startup();
        void Update();
        double GetAbsoluteTime();

        double mStartTime;
        double mElapsedTime;
        double mPreviousTime;
        double mDeltaTime;
        double mClockFrequency;
    };

    class BlitPerformanceCounter
    {
    public:
        void Generate(WorldTimeManager* pClock);
        double Startup();
        double End();
        void Reset();
    private:
        double mStartTime{ 0.0 };
        double mEndTime{ 0.0 };
        double mClockFrequency{ 0.0 };
    };
}

namespace BlitzenPlatform
{
    void PlatfrormSetupClock(BlitzenCore::WorldTimeManager* pClock);
    
    double PlatformGetAbsoluteTime(double frequence);
}