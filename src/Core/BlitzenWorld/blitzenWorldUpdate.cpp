#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	void WV_DRIVE(BLITZEN_SYSTEM_CONTEXT& context)
	{
		if (context.BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::RUNNING)
		{
			context.pWORLD->mResidents.UpdateFallingResidents(context.pWORLD->deltaTime);
			BlitzenWorld::DispatchCollisionSystems(context.pWORLD);
		}
	}
}