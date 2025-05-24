#pragma once
#include "Core/blitzenEngine.h"

namespace BlitzenCore
{
    class WorldTimerManager
    {
    public:

        WorldTimerManager();

        double m_startTime;

        double m_elapsedTime;

        double m_previousTime;

        double m_deltaTime;

        double m_clockFrequency;
    };

    void UpdateWorldClock(WorldTimerManager& clock);
}

namespace BlitzenPlatform
{
    
    void PlatfrormSetupClock(BlitzenCore::WorldTimerManager* pClock);
    
    double PlatformGetAbsoluteTime(double frequence);
}