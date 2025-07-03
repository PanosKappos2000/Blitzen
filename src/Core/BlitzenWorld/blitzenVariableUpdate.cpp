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
		auto& camera = pWORLD->pCameraContainer->GetMainCamera();

		BlitzenCore::UpdateWorldClock(context.pClock);
		context.pWORLD->deltaTime = (float)context.pClock->m_deltaTime;
		camera.viewData.deltaTime = context.pWORLD->deltaTime;

		BLIT_ASSERT(camera.viewData.deltaTime >= 0.f);

		BlitzenPlatform::DispatchEvents(context.pPlatform);

		switch (context.BLITZEN_ENGINE.m_state)
		{
		case BlitzenCore::EngineState::LOADING:
		{
			break;
		}
		case BlitzenCore::EngineState::RUNNING_EDITOR_NO_START:
		{
			BlitzenEngine::UpdateCamera(pWORLD->pCameraContainer->GetMainCamera(), pWORLD->deltaTime);

			pWORLD->DispatchFrameEvents(pWORLD->deltaTime);

			break;
		}
		case BlitzenCore::EngineState::RUNNING:
		{
			BlitzenEngine::UpdateCamera(pWORLD->pCameraContainer->GetMainCamera(), pWORLD->deltaTime);

			pWORLD->DispatchFrameEvents(pWORLD->deltaTime);

			break;
		}
		case BlitzenCore::EngineState::SUSPENDED:
		{
			BlitzenPlatform::DispatchEvents(context.pPlatform);

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
			BlitzenPlatform::DispatchEvents(context.pPlatform);

			break;
		}
		}
	}
}