#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	static void DispatchBlitzenMassivelyParallelRenderer(BlitzenEngine::RendererPtrType pRenderer, BlitzenEngine::WORLD_RESIDENTS& RESIDENTS, BlitzenEngine::Camera& camera)
	{
		BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::PREVIOUS_FRAME);

		BlitzenEngine::UpdateRendererView(pRenderer, camera.viewData, camera.transformData.bFreezeFrustum);

		BlitzenEngine::CULL_CONTEXT cullContext{};
		cullContext.m_cullType = BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION;
		cullContext.m_workType = BlitzenEngine::RENDER_OBJECT_TYPE::OPAQUE_STATIC;
		cullContext.m_workCount = RESIDENTS.m_renders.m_opaqueStaticCount;
		cullContext.m_pResidents = &RESIDENTS;// IS THIS NEEDED?
		BlitzenEngine::DispatchCullingShaders(pRenderer, cullContext);

		BlitzenEngine::UpdateRendererTransforms(pRenderer, RESIDENTS.m_transforms.m_moveables, RESIDENTS.m_transforms.m_moveableCount);

		cullContext.m_cullType = BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION;
		cullContext.m_workType = BlitzenEngine::RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC;
		cullContext.m_workCount = RESIDENTS.m_renders.m_opaqueDynamicCount;
		BlitzenEngine::DispatchCullingShaders(pRenderer, cullContext);

		BlitzenEngine::SetupForFirstRenderPass(pRenderer);
		BlitzenEngine::RENDER_CONTEXT staticRenderContext{};
		staticRenderContext.m_renderType = BlitzenEngine::BLIT_RENDER_TYPE::RENDER_OPAQUE;
		BlitzenEngine::RenderObjects(pRenderer, staticRenderContext);

		BlitzenEngine::RENDER_CONTEXT dynamicRenderContext{};
		dynamicRenderContext.m_renderType = BlitzenEngine::BLIT_RENDER_TYPE::RENDER_DYNAMIC;
		BlitzenEngine::RenderObjects(pRenderer, dynamicRenderContext);

		BlitzenEngine::FinalizeRendering(pRenderer);

		BlitzenEngine::GenerateHI_Z_MAP(pRenderer);

		BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::GRAPHICS);
	}

	void BMPR_DRIVE(BLITZEN_SYSTEM_CONTEXT& context)
	{
		BlitzenEngine::RendererPtrType pRenderer = context.pWORLD->P_RENDERER.Data();
		BlitzenEngine::WORLD_RESIDENTS& RESIDENTS = context.pWORLD->m_residents;
		auto& camera = context.pWORLD->m_cameras[context.pWORLD->m_activeCameraIDX];

		switch (context.BLITZEN_ENGINE.m_state)
		{
		case BlitzenCore::EngineState::RUNNING:
		{
			uint32_t presentCount = 0;
			// 1. Wait for previous frame
			DispatchBlitzenMassivelyParallelRenderer(pRenderer, RESIDENTS, camera);
			presentCount++;

			// TODO: Move the editor no start outside Blitzen's state
			if (context.m_controllerState == ControllerState::Editor)
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