#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	static void DispatchBlitzenMassivelyParallelRenderer(BlitzenEngine::RendererPtrType pRenderer, BlitzenEngine::WORLD_RESIDENTS& RESIDENTS, BlitzenEngine::Camera& camera, uint32_t terrainCount)
	{
		// To avoid the double fence, I have to split the camera data.
		BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::COMPUTE);
		BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::GRAPHICS);

		// Passes camera values to GPU
		BlitzenEngine::UpdateRendererView(pRenderer, camera.viewData, camera.transformData.bFreezeFrustum);

		// Starts with Hierarchical Z Buffer, built with previous frame depth target values
		// Compute queue has started recording at this point
		BlitzenEngine::BeginGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);
		BlitzenEngine::GenerateHI_Z_MAP(pRenderer);

		// Static draws do not have to wait for anything else. Culling begins
		BlitzenEngine::CULL_CONTEXT cullContext{};
		cullContext.m_cullType = BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION;
		cullContext.m_workType = BlitzenEngine::RENDER_OBJECT_TYPE::OPAQUE_STATIC;
		cullContext.m_workCount = RESIDENTS.m_renders.m_opaqueStaticCount;
		cullContext.m_pResidents = &RESIDENTS;// IS THIS NEEDED?
		BlitzenEngine::DispatchCullingShaders(pRenderer, cullContext);
		// End compute commands here, so that the fence can be signaled
		BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);

		// Begins graphics commands and starts the render pass (clears color buffer)
		BlitzenEngine::BeginGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::GRAPHICS);
		BlitzenEngine::SetupForFirstRenderPass(pRenderer);
		BlitzenEngine::RenderTerrain(pRenderer, terrainCount);

		// Waits for static object culling (compute shader0
		BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::COMPUTE);

		// Now draws static render objects
		BlitzenEngine::RENDER_CONTEXT staticRenderContext{};
		staticRenderContext.m_renderType = BlitzenEngine::BLIT_RENDER_TYPE::RENDER_OPAQUE;
		BlitzenEngine::RenderObjects(pRenderer, staticRenderContext);
		
		// Starts transfer commands. The function blocks the culling shader, until the dynamic transforms are updated
		BlitzenEngine::BeginGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::TRANSFER);
		BlitzenEngine::UpdateRendererTransforms(pRenderer, RESIDENTS.m_transforms.WVWithMovement, RESIDENTS.m_transforms.m_moveableCount);

		// Start transforming and culling dynamic objects
		BlitzenEngine::BeginGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);
		cullContext.m_cullType = BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION;
		cullContext.m_workType = BlitzenEngine::RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC;
		cullContext.m_workCount = RESIDENTS.m_renders.m_opaqueDynamicCount;
		BlitzenEngine::DispatchCullingShaders(pRenderer, cullContext);

		// Puts buffers that must be readback after the shader into readback mode and blocks transfer
		// Also blocks graphics until culling shader is done
		BlitzenEngine::ChangeCullingBuffersToReadbackMode(pRenderer);
		BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);
		BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::COMPUTE);

		// Dynamic object graphics
		BlitzenEngine::RENDER_CONTEXT dynamicRenderContext{};
		dynamicRenderContext.m_renderType = BlitzenEngine::BLIT_RENDER_TYPE::RENDER_DYNAMIC;
		BlitzenEngine::RenderObjects(pRenderer, dynamicRenderContext);
		BlitzenEngine::FinalizeRendering(pRenderer);
		BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::GRAPHICS);

		// Pass data back to the CPU for logic updates
		BlitzenEngine::SHADER_GAME_LOGIC_UPDATES shaderDataReadback{};
		shaderDataReadback.m_transformCount = RESIDENTS.m_transforms.m_moveableCount;
		shaderDataReadback.pGpuTransorms = RESIDENTS.m_transforms.WVWithMovement;
		BlitzenEngine::RequestGameLogicUpdatesFromShader(pRenderer, shaderDataReadback);
		BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);
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
			DispatchBlitzenMassivelyParallelRenderer(pRenderer, RESIDENTS, camera, context.pRenderingResources->m_terrainContainer.terrainIndexCount);
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