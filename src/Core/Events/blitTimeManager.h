#pragma once
#include "Core/blitzenEngine.h"

namespace BlitzenCore
{
    class WorldTimeManager
    {
    public:

        WorldTimeManager();

        double m_startTime;

        double m_elapsedTime;

        double m_previousTime;

        double m_deltaTime;

        double m_clockFrequency;
    };

    void UpdateWorldClock(WorldTimeManager& clock);
}

namespace BlitzenPlatform
{
    
    void PlatfrormSetupClock(BlitzenCore::WorldTimeManager* pClock);
    
    double PlatformGetAbsoluteTime(double frequence);
}