#include "blitTimeManager.h"
#include "Platform/blitPlatform.h"

namespace BlitzenCore
{
    WorldTimerManager::WorldTimerManager() : 
        m_elapsedTime{ 0.0 }, m_previousTime{ 0.0 }, m_deltaTime{0.0}
	{
        BlitzenPlatform::PlatfrormSetupClock(this);
        m_startTime = BlitzenPlatform::PlatformGetAbsoluteTime(m_clockFrequency);
    }

    void UpdateWorldClock(WorldTimerManager& clock)
    {
        clock.m_elapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime(clock.m_clockFrequency) - clock.m_startTime;
        clock.m_deltaTime = clock.m_elapsedTime - clock.m_previousTime;
        clock.m_previousTime = clock.m_elapsedTime;
    }
}