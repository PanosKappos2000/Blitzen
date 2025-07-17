#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	void BMPR_DRIVE(BLITZEN_SYSTEM_CONTEXT& context)
	{
		BlitzenEngine::RendererPtrType pRenderer = context.pWORLD->BMPR.Data();
		BlitzenEngine::WORLD_RESIDENTS& RESIDENTS = context.pWORLD->mResidents;
		auto& camera = context.pWORLD->m_cameras[context.pWORLD->m_activeCameraIDX];

		switch (context.BLITZEN_ENGINE.m_state)
		{
		case BlitzenCore::EngineState::RUNNING:
		{
			uint32_t presentCount = 0;
			DispatchBumper(context.pWORLD, context.pRenderingResources->m_terrainContainer.terrainIndexCount);
			presentCount++;

			// TODO: Move the editor no start outside Blitzen's state
			if (context.m_controllerState == ControllerState::Editor)
			{
			#if defined(DASHER_JOIN)
				context.pDasher->Draw(context.pWORLD->deltaTime);
				BlitzenEngine::PresentRender(context.pWORLD->BMPR.Data(), 2);
				presentCount++;
			#endif
			}
			BlitzenEngine::PresentRender(pRenderer, presentCount);

			break;
		}
		case BlitzenCore::EngineState::LOADING:
		{
			BlitzenEngine::RendererWorkIdle(pRenderer, BlitzenEngine::RENDERER_IDLE_MODE::BLITZEN_LOGO);
			BlitzenEngine::PresentRender(pRenderer, 1);

			break;
		}
		default:
		{
			break;
		}
		}
	}
}