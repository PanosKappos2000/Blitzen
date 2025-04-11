#include "blitTimeManager.h"

namespace BlitzenCore
{
    WorldTimerManager* WorldTimerManager::s_pThis{nullptr};

    WorldTimerManager::WorldTimerManager() : 
        m_elapsedTime{ 0.0 }, m_previousTime{ 0.0 }, m_deltaTime{0.0}
	{
        BlitzenPlatform::PlatfrormSetupClock();
        m_startTime = BlitzenPlatform::PlatformGetAbsoluteTime();
		s_pThis = this;
    }
}