#pragma once

#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	static void UpdateWVs(BLITZEN_SYSTEM_CONTEXT& context, float deltaTime)
	{
		
	}

	void WorldLoop(BLITZEN_SYSTEM_CONTEXT& context)
	{
		BlitzenCore::UpdateWorldClock(context.pClock);
		auto pWORLD = context.pWORLD;
		context.pWORLD->deltaTime = (float)context.pClock->m_deltaTime;

		switch (context.BLITZEN_ENGINE.m_state)
		{
		case BlitzenCore::EngineState::LOADING:
		{
			break;
		}
		case BlitzenCore::EngineState::RUNNING_EDITOR_NO_START:
		{
			if (!BlitzenPlatform::DispatchEvents(context.pPlatform))
			{
				context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SHUTDOWN;
			}

			BlitzenEngine::UpdateCamera(pWORLD->pCameraContainer->GetMainCamera(), pWORLD->deltaTime);

			pWORLD->DispatchFrameEvents(pWORLD->deltaTime);

			break;
		}
		case BlitzenCore::EngineState::RUNNING:
		{
			if (!BlitzenPlatform::DispatchEvents(context.pPlatform))
			{
				context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SHUTDOWN;
			}

			BlitzenEngine::UpdateCamera(pWORLD->pCameraContainer->GetMainCamera(), pWORLD->deltaTime);

			pWORLD->DispatchFrameEvents(pWORLD->deltaTime);

			break;
		}
		case BlitzenCore::EngineState::SUSPENDED:
		{
			if (!BlitzenPlatform::DispatchEvents(context.pPlatform))
			{
				context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::SHUTDOWN;
			}

			break;
		}
		case BlitzenCore::EngineState::SETUP_AFTER_LOAD:
		{
			BlitzenEngine::PrepareRendererForRuntime(context.pWORLD->P_RENDERER.Data());

			context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::RUNNING;

			break;
		}
		}
	}
}