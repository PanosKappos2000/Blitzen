#pragma once
#include "Core/blitzenEngine.h"
#include "Platform/platform.h"

namespace BlitzenPlatform
{
    // Sets up clock frequency
    void PlatfrormSetupClock();
    // Get delta time based on the above
    double PlatformGetAbsoluteTime();
}

namespace BlitzenCore
{
    class WorldTimerManager
    {
    public:

        WorldTimerManager();

        inline void Update()
        {
            m_elapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - m_startTime;
            m_deltaTime = m_elapsedTime - m_previousTime;
            m_previousTime = m_elapsedTime;
        }

        inline double GetDeltaTime()
        {
            return m_deltaTime;
        }

    private:

        double m_startTime;

        double m_elapsedTime;

        double m_previousTime;

        double m_deltaTime;
    };
}