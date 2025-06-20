#pragma once

#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	static void UpdateWVs(BlitzenPrivateContext& context, float deltaTime)
	{
		
	}

	void WorldLoop(BlitzenPrivateContext& context)
	{
		BlitzenCore::UpdateWorldClock(*context.pClock);
		context.pWORLD->deltaTime = (float)context.pClock->m_deltaTime;

		switch (*context.pEngineState)
		{
		case BlitzenCore::EngineState::LOADING:
		{
			break;
		}
		case BlitzenCore::EngineState::RUNNING_EDITOR_NO_START:
		{
			if (!BlitzenPlatform::DispatchEvents(context.pPlatform))
			{
				*context.pEngineState = BlitzenCore::EngineState::SHUTDOWN;
			}

			// TODO: Does NOT belong here
			BlitzenEngine::UpdateCamera(context.pWORLD->pCameraContainer->GetMainCamera(), context.pWORLD->deltaTime);

			/*while (*context.pEngineState != BlitzenCore::EngineState::SHUTDOWN)
			{
				UpdateWVs(context, context.pClock->m_deltaTime);
			}*/
			break;
		}
		case BlitzenCore::EngineState::RUNNING:
		{
			if (!BlitzenPlatform::DispatchEvents(context.pPlatform))
			{
				*context.pEngineState = BlitzenCore::EngineState::SHUTDOWN;
			}

			// TODO: Does NOT belong here
			BlitzenEngine::UpdateCamera(context.pWORLD->pCameraContainer->GetMainCamera(), context.pWORLD->deltaTime);

			break;
		}
		case BlitzenCore::EngineState::SUSPENDED:
		{
			if (!BlitzenPlatform::DispatchEvents(context.pPlatform))
			{
				*context.pEngineState = BlitzenCore::EngineState::SHUTDOWN;
			}

			break;
		}
		case BlitzenCore::EngineState::SETUP_AFTER_LOAD:
		{
			BlitzenEngine::PrepareRendererForRuntime(context.pWORLD->P_RENDERER.Data());

			*context.pEngineState = BlitzenCore::EngineState::RUNNING;

			break;
		}
		}
	}
}