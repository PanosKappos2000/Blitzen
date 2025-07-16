#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	void WV_DRIVE(BLITZEN_SYSTEM_CONTEXT& context)
	{
		if (context.BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::RUNNING)
		{
			context.pWORLD->m_residents.UpdateFallingResidents(context.pWORLD->deltaTime);
		}
	}
}