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

        double m_startTime;
        double m_elapsedTime;
        double m_previousTime;
        double m_deltaTime;
        double m_clockFrequency;
    };
}

namespace BlitzenPlatform
{
    
    void PlatfrormSetupClock(BlitzenCore::WorldTimeManager* pClock);
    
    double PlatformGetAbsoluteTime(double frequence);
}