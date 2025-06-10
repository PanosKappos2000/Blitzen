#pragma once

#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	static void UpdateWVs(BlitzenPrivateContext& context, float deltaTime)
	{
		for (uint32_t var = 0; var < context.pBlitzenContext->wvCount; ++var)
		{
			context.pBlitzenContext->WVs[var].PFNTICK(context.pBlitzenContext->WVs[var].pWVDATA, context.pClock->m_deltaTime);
		}
	}

	void WorldLoop(BlitzenPrivateContext& context)
	{
		if (!BlitzenPlatform::DispatchEvents(context.pPlatform))
		{
			*context.pEngineState = BlitzenCore::EngineState::SHUTDOWN;
		}

		BlitzenCore::UpdateWorldClock(*context.pClock);
		context.pBlitzenContext->deltaTime = context.pClock->m_deltaTime;

		// TODO: Does NOT belong here
		BlitzenEngine::UpdateCamera(context.pBlitzenContext->pCameraContainer->GetMainCamera(), context.pClock->m_deltaTime);

		while (*context.pEngineState != BlitzenCore::EngineState::SHUTDOWN)
		{
			UpdateWVs(context, context.pClock->m_deltaTime);
		}
	}
}