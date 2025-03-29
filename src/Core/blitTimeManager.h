#include "Engine/blitzenEngine.h"
#include "Platform/platform.h"

namespace BlitzenCore
{
    class WorldTimerManager
    {
    public:

        WorldTimerManager();

        inline double GetDeltaTime()
        {
			m_elapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - m_startTime;
            double deltaTime = m_elapsedTime - m_previousTime;
            m_previousTime = m_elapsedTime;
            return deltaTime;
        }

    private:

        double m_startTime;

        double m_elapsedTime;

        double m_previousTime;

    public:
		inline static WorldTimerManager* GetInstance(){return s_pThis;}

    private:
		static WorldTimerManager* s_pThis;
    };
}