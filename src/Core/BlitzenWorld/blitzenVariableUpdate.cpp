#pragma once
#include "blitzenWorldPrivate.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenWorld
{
	static void UpdateWVs(BLITZEN_SYSTEM_CONTEXT& context, float deltaTime)
	{
		
	}

	void WorldLoop(BLITZEN_SYSTEM_CONTEXT& context)
	{
		auto pWORLD = context.pWORLD;
		auto& camera = context.pWORLD->m_cameras[context.pWORLD->m_activeCameraIDX];

		context.pClock->Update();
		context.pWORLD->deltaTime = (float)context.pClock->m_deltaTime;
		camera.viewData.deltaTime = context.pWORLD->deltaTime;

		BLIT_ASSERT(camera.viewData.deltaTime >= 0.f && camera.viewData.deltaTime <= BlitzenCore::CE_MAX_TIME_STEP);

		BlitzenPlatform::DispatchEvents(context.pPlatform);

		switch (context.BLITZEN_ENGINE.m_state)
		{
		case BlitzenCore::EngineState::LOADING:
		{
			break;
		}
		case BlitzenCore::EngineState::RUNNING:
		{
			if (context.m_controllerState != ControllerState::Editor)
			{
				pWORLD->DispatchFrameEvents(pWORLD->deltaTime);
			}
			break;
		}
		case BlitzenCore::EngineState::SUSPENDED:
		{
			break;
		}
		case BlitzenCore::EngineState::SETUP_AFTER_LOAD:
		{
			BlitzenEngine::PrepareRendererForRuntime(context.pWORLD->P_RENDERER.Data());

			context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::RUNNING;

			break;
		}
		default:
		{
			break;
		}
		}
	}
}