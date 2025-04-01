#include "blitTimeManager.h"

namespace BlitzenCore
{
    WorldTimerManager* WorldTimerManager::s_pThis{nullptr};

    WorldTimerManager::WorldTimerManager() :
        m_startTime{ BlitzenPlatform::PlatformGetAbsoluteTime() }, 
        m_elapsedTime{ 0.0 }, m_previousTime{ 0.0 }, m_deltaTime{0.0}
	{
		s_pThis = this;
    }
}