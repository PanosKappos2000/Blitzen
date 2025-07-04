#include "blitzenSystemDispatcher.h"

namespace BlitzenWorld
{
	inline static JobDriverQueue* P_JOB_SYSTEM = nullptr;

	void StartJobSystem(JobDriverQueue* ptr)
	{
		P_JOB_SYSTEM = ptr;
	}

	bool RequestSystemJobDrive(ENGINE_SYSTEM_DRIVE_REQUEST req)
	{
		if (!P_JOB_SYSTEM->Register(req))
		{
			P_JOB_SYSTEM->m_state = JobDriverState::HELP_NEEDED;
			return false;
		}

		return true;
	}
}