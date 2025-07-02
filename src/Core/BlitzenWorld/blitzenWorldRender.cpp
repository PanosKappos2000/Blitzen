#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	static void DispatchBlitzenMassivelyParallelRenderer(BlitzenEngine::RendererPtrType pRenderer, BlitzenEngine::WORLD_RESIDENTS& RESIDENTS, BlitzenEngine::Camera& camera)
	{
		BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::PREVIOUS_FRAME);

		// 2. Wait for camera and update view
		BlitzenEngine::UpdateRendererView(pRenderer, camera.viewData, camera.transformData.bFreezeFrustum);

		// 3. Cull static objects
		BlitzenEngine::CULL_CONTEXT cullContext{};
		cullContext.m_cullType = BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION;
		cullContext.m_workType = BlitzenEngine::RENDER_OBJECT_TYPE::OPAQUE_STATIC;
		cullContext.m_workCount = RESIDENTS.m_renders.m_opaqueStaticCount;
		cullContext.m_pResidents = &RESIDENTS;// IS THIS NEEDED?
		BlitzenEngine::DispatchCullingShaders(pRenderer, cullContext);

		// 4. Wait for game logic and update transforms
		BlitzenEngine::UpdateRendererTransforms(pRenderer, RESIDENTS.m_transforms.m_moveables, RESIDENTS.m_transforms.m_moveableCount);

		// 5. Cull Moving objects
		cullContext.m_cullType = BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION;
		cullContext.m_workType = BlitzenEngine::RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC;
		cullContext.m_workCount = RESIDENTS.m_renders.m_opaqueDynamicCount;
		BlitzenEngine::DispatchCullingShaders(pRenderer, cullContext);

		// 6. Draw
		BlitzenEngine::SetupForFirstRenderPass(pRenderer);
		BlitzenEngine::RENDER_CONTEXT renderContext[2]{};
		renderContext[0].m_renderType = BlitzenEngine::BLIT_RENDER_TYPE::RENDER_OPAQUE;
		renderContext[1].m_renderType = BlitzenEngine::BLIT_RENDER_TYPE::RENDER_DYNAMIC;
		BlitzenEngine::RenderObjects(pRenderer, renderContext, 2);

		// 7. Generate HI_Z_MAP
		BlitzenEngine::GenerateHI_Z_MAP(pRenderer);

		// Extra. Wait for draw, cull transparents and draw them. This would move the HI_Z_MAP step

		BlitzenEngine::FinalizeRendering(pRenderer);
	}

	void BMPR_DRIVE(BLITZEN_SYSTEM_CONTEXT& context)
	{
		BlitzenEngine::RendererPtrType pRenderer = context.pWORLD->P_RENDERER.Data();
		BlitzenEngine::WORLD_RESIDENTS& RESIDENTS = context.pWORLD->m_residents;
		auto& camera = context.pWORLD->pCameraContainer->GetMainCamera();

		switch (context.BLITZEN_ENGINE.m_state)
		{
		case BlitzenCore::EngineState::RUNNING_EDITOR_NO_START:
		case BlitzenCore::EngineState::RUNNING:
		{
			uint32_t presentCount = 0;
			// 1. Wait for previous frame
			DispatchBlitzenMassivelyParallelRenderer(pRenderer, RESIDENTS, camera);
			presentCount++;

			// TODO: Move the editor no start outside Blitzen's state
			if (context.BLITZEN_ENGINE.m_state != BlitzenCore::EngineState::RUNNING_EDITOR_NO_START)
			{
			#if defined(DASHER_JOIN)
				context.pDasher->Draw(context.pWORLD->deltaTime);
				BlitzenEngine::PresentRender(context.pWORLD->P_RENDERER.Data(), 2);
				presentCount++;
			#endif
			}
			BlitzenEngine::PresentRender(pRenderer, presentCount);

			break;
		}
		case BlitzenCore::EngineState::LOADING:
		{
			BlitzenEngine::RendererWorkIdle(pRenderer, BlitzenEngine::RENDERER_IDLE_MODE::TRIANGLE);
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