#include "blitTimeManager.h"
#include "Platform/blitPlatform.h"

namespace BlitzenCore
{
    WorldTimeManager::WorldTimeManager() :
        m_elapsedTime{ 0.0 }, m_previousTime{ 0.0 }, m_deltaTime{0.0}
	{
        BlitzenPlatform::PlatfrormSetupClock(this);
    }

    void WorldTimeManager::Startup()
    {
        m_startTime = BlitzenPlatform::PlatformGetAbsoluteTime(m_clockFrequency);
    }

    void WorldTimeManager::Update()
    {
        m_elapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime(m_clockFrequency) - m_startTime;
        m_deltaTime = m_elapsedTime - m_previousTime <= CE_MAX_TIME_STEP ? m_elapsedTime - m_previousTime : CE_MAX_TIME_STEP;
        m_previousTime = m_elapsedTime;
    }
}