#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	void WV_DRIVE(BLITZEN_SYSTEM_CONTEXT& context)
	{
		if (context.BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::RUNNING)
		{
			if (context.m_controllerState != ControllerState::Editor)
			{
				//BlitzenEngine::UpdateCamera(context.pWORLD->m_cameras[context.pWORLD->m_activeCameraIDX], context.pWORLD->deltaTime);
				BlitzenEngine::UpdateResidentAttachedCamera(context.pWORLD->m_cameras[context.pWORLD->m_activeCameraIDX], context.pWORLD->deltaTime);
			}
		}
	}
}