#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	void RenderLoop(BLITZEN_SYSTEM_CONTEXT& context)
	{
		
			// TODO: CULL STATIC OBJECTS FIRST

			// TODO: Update view after waiting for camera

			// TODO: Wait for update loop
			switch (context.BLITZEN_ENGINE.m_state)
			{
			case BlitzenCore::EngineState::RUNNING_EDITOR_NO_START:
			{
				context.pWORLD->P_RENDERER->DrawFrame(context.pWORLD->m_drawContext);

				context.pWORLD->P_RENDERER->Present();

				break;
			}
			case BlitzenCore::EngineState::RUNNING:
			{
				context.pWORLD->P_RENDERER->DrawFrame(context.pWORLD->m_drawContext);

#if defined(DASHER_JOIN)

				context.pDasher->Draw(context.pWORLD->deltaTime);

#endif

				context.pWORLD->P_RENDERER->Present();

				break;
			}
			case BlitzenCore::EngineState::LOADING:
			{
				context.pWORLD->P_RENDERER->DrawWhileWaiting(context.pWORLD->deltaTime);

				break;
			}
			default:
			{
				break;
			}
			}
		
	}
}