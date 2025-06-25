#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	void RenderLoop(BLITZEN_SYSTEM_CONTEXT& context)
	{
			BlitzenEngine::RendererPtrType pRenderer = context.pWORLD->P_RENDERER.Data();
			auto& camera = context.pWORLD->pCameraContainer->GetMainCamera();

			switch (context.BLITZEN_ENGINE.m_state)
			{
			case BlitzenCore::EngineState::RUNNING_EDITOR_NO_START:
			case BlitzenCore::EngineState::RUNNING:
			{
				// 1. Wait for previous frame
				BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::PREVIOUS_FRAME);

				// 2. Wait for camera and update view
				BlitzenEngine::UpdateRendererView(pRenderer, camera.viewData, camera.transformData.bFreezeFrustum);

				// 3. Cull static objects
				BlitzenEngine::CULL_CONTEXT cullContext{};
				cullContext.m_cullType = BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION;
				cullContext.m_workType = BlitzenEngine::RENDER_OBJECT_TYPE::OPAQUE_STATIC;
				cullContext.m_workCount = context.pWORLD->m_residents.m_renders.m_opaqueStaticCount;
				cullContext.m_pResidents = &context.pWORLD->m_residents;// IS THIS NEEDED?
				BlitzenEngine::DispatchCullingShaders(pRenderer, cullContext);

				// 4. Wait for game logic and update transforms
				BlitzenEngine::UpdateRendererTransforms(pRenderer);

				// 5. Cull Moving objects

				// 6. Draw
				BlitzenEngine::SetupForFirstRenderPass(pRenderer);
				BlitzenEngine::RENDER_CONTEXT renderContext{};
				renderContext.m_renderType = BlitzenEngine::BLIT_RENDER_TYPE::RENDER_OPAQUE;
				BlitzenEngine::RenderObjects(pRenderer, renderContext);
				//pRenderer->DrawFrame(context.pWORLD->m_drawContext);

				// 7. Generate HI_Z_MAP
				BlitzenEngine::GenerateHI_Z_MAP(pRenderer);

				// Extra. Wait for draw, cull transparents and draw them. This would move the HI_Z_MAP step

				BlitzenEngine::FinalizeRendering(pRenderer);

				// TODO: Move the editor no start outside Blitzen's state
				if (context.BLITZEN_ENGINE.m_state != BlitzenCore::EngineState::RUNNING_EDITOR_NO_START)
				{
				#if defined(DASHER_JOIN)
					context.pDasher->Draw(context.pWORLD->deltaTime);
					BlitzenEngine::PresentRender(context.pWORLD->P_RENDERER.Data(), 2);
				#else
					BlitzenEngine::PresentRender(pRenderer, 1);
				#endif
				}
				else
				{
					BlitzenEngine::PresentRender(context.pWORLD->P_RENDERER.Data(), 1);
				}

				break;
			}
			case BlitzenCore::EngineState::LOADING:
			{
				context.pWORLD->P_RENDERER->DrawWhileWaiting(context.pWORLD->deltaTime);
				BlitzenEngine::PresentRender(context.pWORLD->P_RENDERER.Data(), 1);

				break;
			}
			default:
			{
				break;
			}
			}
		
	}
}