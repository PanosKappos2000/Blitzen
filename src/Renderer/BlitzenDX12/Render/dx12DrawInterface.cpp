#if defined(_WIN32)
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"

namespace BlitzenEngine
{
	void RenderObjects(BlitzenDX12::Dx12Renderer* pRenderer, RENDER_CONTEXT& renderContext)
	{
		UINT frame{ pRenderer->m_currentFrame };
		auto& cmd{ pRenderer->m_cmdContext[frame] };
		auto cmdList{ cmd.m_graphicsCmdList };
		auto& descriptorContext{ pRenderer->m_descriptorContext };
		auto& pipelineContext{ pRenderer->m_pipelineContext };
		auto& rwResources{ pRenderer->m_rwResources[frame] };

		switch (renderContext.m_renderType)
		{
		case BLIT_RENDER_TYPE::RENDER_OPAQUE:
		{
			// Pipeline
			cmdList->SetPipelineState(pipelineContext.m_staticDrawPso.Get());
			// Primitives
			cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			// Index buffer
			cmdList->IASetIndexBuffer(&pRenderer->m_roResources.m_idxBuffer.m_view);
			// DRAW
			cmdList->ExecuteIndirect(pipelineContext.m_staticDrawCmdSignature.Get(), BLIT_MAX_STATIC_DRAW_COMMANDS, rwResources.m_staticDrawCmdBuffer.buffer.Get(),
				0, rwResources.m_staticDrawCmdCounter.buffer.Get(), 0);

			break;
		}
		case BLIT_RENDER_TYPE::RENDER_DYNAMIC:
		{
			// Pipeline
			cmdList->SetPipelineState(pipelineContext.m_dynamicDrawPso.Get());
			// Primitives
			cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			// Index buffer
			cmdList->IASetIndexBuffer(&pRenderer->m_roResources.m_idxBuffer.m_view);
			// DRAW
			cmdList->ExecuteIndirect(pipelineContext.m_dynamicDrawCmdSignature.Get(), BLIT_MAX_DYNAMIC_DRAW_COMMANDS, rwResources.m_dynamicDrawCmdBuffer.buffer.Get(),
				0, rwResources.m_dynamicDrawCmdCounter.buffer.Get(), 0);
			break;
		}
		default:
		{
			break;
		}
		}
	}

	void SetupForFirstRenderPass(BlitzenDX12::Dx12Renderer* pRenderer)
	{
		UINT frame{ pRenderer->m_currentFrame };
		pRenderer->m_swapchainIDX = pRenderer->m_swapchain->GetCurrentBackBufferIndex();
		UINT swapchainIDX{ pRenderer->m_swapchainIDX };
		auto& cmd{ pRenderer->m_cmdContext[frame] };
		auto renderTarget{ pRenderer->m_swapchainBackBuffers[swapchainIDX].Get() };
		auto& descriptorContext{ pRenderer->m_descriptorContext };
		auto cmdList{ cmd.m_graphicsCmdList };

		// Render target barrier
		D3D12_RESOURCE_BARRIER renderTargetBarriers[2]{};
		BlitzenDX12::CreateResourcesTransitionBarrier(renderTargetBarriers[0], pRenderer->m_swapchainBackBuffers[swapchainIDX].Get(), 
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		BlitzenDX12::CreateResourcesTransitionBarrier(renderTargetBarriers[1], pRenderer->m_depthBuffers[swapchainIDX].Get(), 
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		cmd.m_graphicsCmdList->ResourceBarrier(BLIT_ARRAY_SIZE(renderTargetBarriers), renderTargetBarriers);

		// Viewport and scissor
		BlitzenDX12::DefineViewportAndScissor(cmd.m_graphicsCmdList.Get(), (float)pRenderer->m_swapchainWidth, (float)pRenderer->m_swapchainHeight);

		cmd.m_graphicsCmdList->OMSetRenderTargets(1, &descriptorContext.m_swapchainRtvHandle[swapchainIDX], FALSE, &descriptorContext.m_depthTargetDSVHandle[swapchainIDX]);
		cmd.m_graphicsCmdList->ClearRenderTargetView(descriptorContext.m_swapchainRtvHandle[swapchainIDX], BlitzenCore::Ce_DefaultWindowBackgroundColor, 0, nullptr);
		cmd.m_graphicsCmdList->ClearDepthStencilView(descriptorContext.m_depthTargetDSVHandle[swapchainIDX], D3D12_CLEAR_FLAG_DEPTH, BlitzenDX12::Ce_ClearDepth, 0, 0, nullptr);

		// DESCRIPTORS
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_VTX_TABLE_ID, descriptorContext.m_vertexODSTableHandle[frame]);
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_GLOBAL_ID, descriptorContext.mGlobalDescriptorsTableHandle[frame]);
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_TEX_ID, descriptorContext.m_texturesTableHandle);
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_TEXSMP_ID, descriptorContext.m_texSmpHandle);
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_PS_TABLE_ID, descriptorContext.m_pixelODSTableHandle);
	}

	void FinalizeRendering(BlitzenDX12::Dx12Renderer* pRenderer)
	{
		auto& cmd{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };

		D3D12_RESOURCE_BARRIER hi_z_mapBarriers[1]{};
		BlitzenDX12::CreateResourcesTransitionBarrier(hi_z_mapBarriers[0], pRenderer->m_depthBuffers[pRenderer->m_swapchainIDX].Get(), 
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cmd.m_graphicsCmdList->ResourceBarrier(1, hi_z_mapBarriers);

		D3D12_RESOURCE_BARRIER presentBarriers[1]{};
		BlitzenDX12::CreateResourcesTransitionBarrier(presentBarriers[0], pRenderer->m_swapchainBackBuffers[pRenderer->m_swapchainIDX].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cmd.m_graphicsCmdList->ResourceBarrier(BLIT_ARRAY_SIZE(presentBarriers), presentBarriers);
	}

	void RenderTerrain(BlitzenDX12::Dx12Renderer* pRenderer, uint32_t terrainCount)
	{
		auto cmdList = pRenderer->m_cmdContext[pRenderer->m_currentFrame].m_graphicsCmdList;
		auto& rwResources = pRenderer->m_rwResources[pRenderer->m_currentFrame];

		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_TERRAIN_VERTICES_ID, pRenderer->m_descriptorContext.m_terrainVertexTableHandle);

		// Pipeline
		cmdList->SetPipelineState(pRenderer->m_pipelineContext.m_terrainDrawPso.Get());
		// Primitives
		cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// Index buffer
		cmdList->IASetIndexBuffer(&pRenderer->m_roResources.m_terrainIdxBuffer.m_view);
		// DRAW
		cmdList->DrawIndexedInstanced(terrainCount, 1, 0, 0, 0);
	}
#if !defined(NDEBUG)
	void RENDER_BOUNDING_SPHERES_DEBUG(BlitzenDX12::Dx12Renderer* pRenderer)
	{
		auto cmdList = pRenderer->m_cmdContext[pRenderer->m_currentFrame].m_graphicsCmdList;
	}
#endif

	void RendererWorkIdle(BlitzenDX12::Dx12Renderer* pRenderer, RENDERER_IDLE_MODE mode)
	{
		UINT frame{ pRenderer->m_currentFrame };
		auto& cmdContext = pRenderer->m_cmdContext[frame];
		auto& pipelineContext = pRenderer->m_pipelineContext;
		auto& descriptorContext = pRenderer->m_descriptorContext;
		float swapchainWidth{ (float)pRenderer->m_swapchainWidth };
		float swapchainHeight{ (float)pRenderer->m_swapchainHeight };

		BlitzenDX12::PlaceFence(cmdContext.m_frameFence.m_value, pRenderer->m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);
		UINT swapchainIDX{ pRenderer->m_swapchain->GetCurrentBackBufferIndex()};
		auto swapchainBackBuffer = pRenderer->m_swapchainBackBuffers[swapchainIDX].Get();

		pRenderer->m_swapchainIDX = swapchainIDX;

		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), nullptr);

		ID3D12DescriptorHeap* graphicsHeaps[] = { descriptorContext.m_viewHeap.Get(), descriptorContext.m_samplerHeap.Get() };
		cmdContext.m_graphicsCmdList->SetDescriptorHeaps(2, graphicsHeaps);

		switch (mode)
		{
		case RENDERER_IDLE_MODE::TRIANGLE:
		{
			cmdContext.m_graphicsCmdList->SetGraphicsRootSignature(pipelineContext.m_triangleRoot.Get());
			BlitzenDX12::DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)swapchainWidth, (float)swapchainHeight);

			D3D12_RESOURCE_BARRIER attachmentBarrier{};
			BlitzenDX12::CreateResourcesTransitionBarrier(attachmentBarrier, swapchainBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &attachmentBarrier);

			cmdContext.m_graphicsCmdList->OMSetRenderTargets(1, &descriptorContext.m_swapchainRtvHandle[swapchainIDX], FALSE, nullptr);

			cmdContext.m_graphicsCmdList->ClearRenderTargetView(descriptorContext.m_swapchainRtvHandle[swapchainIDX], BlitzenCore::Ce_DefaultWindowBackgroundColor, 0, nullptr);

			//BlitML::vec3 triangleColor{ 0, 0.8f, 0.4f };
			//frameTools.mainGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 3, &triangleColor, 0);
			cmdContext.m_graphicsCmdList->SetPipelineState(pipelineContext.m_trianglePso.Get());
			cmdContext.m_graphicsCmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdContext.m_graphicsCmdList->DrawInstanced(3, 1, 0, 0);
			break;
		}
		case RENDERER_IDLE_MODE::BLITZEN_LOGO:
		{
			cmdContext.m_graphicsCmdList->SetGraphicsRootSignature(pipelineContext.m_blitzenLogoRoot.Get());
			BlitzenDX12::DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)swapchainWidth, (float)swapchainHeight);

			D3D12_RESOURCE_BARRIER attachmentBarrier{};
			BlitzenDX12::CreateResourcesTransitionBarrier(attachmentBarrier, swapchainBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &attachmentBarrier);

			cmdContext.m_graphicsCmdList->OMSetRenderTargets(1, &descriptorContext.m_swapchainRtvHandle[swapchainIDX], FALSE, nullptr);

			cmdContext.m_graphicsCmdList->ClearRenderTargetView(descriptorContext.m_swapchainRtvHandle[swapchainIDX], BlitzenCore::Ce_DefaultWindowBackgroundColor, 0, nullptr);

			cmdContext.m_graphicsCmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_BLITZEN_LOGO_TEX_ID, descriptorContext.m_blitzenLogoTextureTableHandle);
			cmdContext.m_graphicsCmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_BLITZEN_LOGO_SAMPLER_ID, descriptorContext.m_texSmpHandle);

			cmdContext.m_graphicsCmdList->SetPipelineState(pipelineContext.m_blitzenLogoPipelineState.Get());
			cmdContext.m_graphicsCmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdContext.m_graphicsCmdList->DrawInstanced(4, 1, 0, 0);

			break;
		}
		}

		D3D12_RESOURCE_BARRIER presentBarrier{};
		BlitzenDX12::CreateResourcesTransitionBarrier(presentBarrier, swapchainBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &presentBarrier);

		cmdContext.m_graphicsCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_graphicsCmdList.Get() };
		pRenderer->m_commandQueue->ExecuteCommandLists(1, commandLists);
	}
}

#endif