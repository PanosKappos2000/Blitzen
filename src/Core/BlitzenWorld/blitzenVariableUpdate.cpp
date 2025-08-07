#pragma once
#include "blitzenWorldPrivate.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/Events/blitEvents.h"
#include "Renderer/Entities/Residents/blitWVScan.h"

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
		context.pWORLD->deltaTime = (float)context.pClock->mDeltaTime;
		camera.viewData.deltaTime = context.pWORLD->deltaTime;

		BLIT_ASSERT(camera.viewData.deltaTime >= 0.f && camera.viewData.deltaTime <= BlitzenCore::GCMaxTimeStep);

		// Collects platform messages
		BlitzenPlatform::DispatchEvents(context.pPlatform);
		// Dispatches event callbacks based on the platform messages
		BlitzenCore::DispatchUserEvents(&context);
		// Checks for held down keys
		context.m_controllers[context.m_activeControllerIDX].DispatchHeldDownKeyEvents(pWORLD->deltaTime);
		if (context.m_controllerState != ControllerState::Game)
		{
			BlitzenEngine::UpdateCamera(context.pWORLD->m_cameras[context.pWORLD->m_activeCameraIDX], context.pWORLD->deltaTime);
		}
		else
		{
			BlitzenEngine::UpdateResidentAttachedCamera(context.pWORLD->m_cameras[context.pWORLD->m_activeCameraIDX], context.pWORLD->deltaTime);
		}

		if (context.BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::RUNNING)
		{
			BlitzenEngine::WorldTickLogic(&pWORLD->mResidents, pWORLD->deltaTime);
			pWORLD->mResidents.UpdateMovingResidents(pWORLD->deltaTime);
			pWORLD->mResidents.UpdateFallingResidents(context.pWORLD->deltaTime);
			BlitzenWorld::DispatchCollisionSystems(context.pWORLD);
		}
		else if (context.BLITZEN_ENGINE.m_state == BlitzenCore::EngineState::SETUP_AFTER_LOAD)
		{
			BLIT_DBLOG("Prepare");
			BlitzenEngine::PrepareRendererForRuntime(context.pWORLD->BMPR.Data());
			BLIT_DBLOG("Out of prepare");
			context.BLITZEN_ENGINE.m_state = BlitzenCore::EngineState::RUNNING;
		}
	}
}