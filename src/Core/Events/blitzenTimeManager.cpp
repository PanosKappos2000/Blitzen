#include "blitTimeManager.h"
#include "Platform/blitPlatform.h"

namespace BlitzenCore
{
    WorldTimeManager::WorldTimeManager() :
        m_elapsedTime{ 0.0 }, m_previousTime{ 0.0 }, m_deltaTime{0.0}
	{
        BlitzenPlatform::PlatfrormSetupClock(this);
        m_startTime = BlitzenPlatform::PlatformGetAbsoluteTime(m_clockFrequency);
    }

    void UpdateWorldClock(WorldTimeManager* pClock)
    {
        pClock->m_elapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime(pClock->m_clockFrequency) - pClock->m_startTime;
        pClock->m_deltaTime = pClock->m_elapsedTime - pClock->m_previousTime;
        pClock->m_previousTime = pClock->m_elapsedTime;
    }
}