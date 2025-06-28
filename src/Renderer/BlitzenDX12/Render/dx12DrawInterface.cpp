#if defined(_WIN32)
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"

namespace BlitzenEngine
{
	void RenderObjects(BlitzenDX12::Dx12Renderer* pRenderer, const RENDER_CONTEXT* renderContextArr, uint32_t renderContextCount)
	{
		UINT frame{ pRenderer->m_currentFrame };
		UINT swapchainIDX{ pRenderer->m_swapchainIDX };
		auto& cmd{ pRenderer->m_cmdContext[frame] };
		auto cmdList{ cmd.m_graphicsCmdList };
		auto renderTarget{ pRenderer->m_swapchainBackBuffers[swapchainIDX].Get()};
		auto& descriptorContext{ pRenderer->m_descriptorContext };
		auto& pipelineContext{ pRenderer->m_pipelineContext };
		auto& rwResources{ pRenderer->m_rwResources[frame] };

		ID3D12DescriptorHeap* graphicsHeaps[] = { descriptorContext.m_viewHeap.Get(), descriptorContext.m_samplerHeap.Get() };
		cmdList->SetDescriptorHeaps(2, graphicsHeaps);
		cmdList->SetGraphicsRootSignature(pipelineContext.m_graphicsRoot.Get());

		// DESCRIPTORS
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_VTX_TABLE_ID, descriptorContext.m_vertexODSTableHandle[frame]);
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_GLOBAL_ID, descriptorContext.m_globalTableHandle[frame]);
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_TEX_ID, descriptorContext.m_texturesTableHandle);
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_TEXSMP_ID, descriptorContext.m_texSmpHandle);
		cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::CE_GRAPHICS_ODS_PS_TABLE_ID, descriptorContext.m_pixelODSTableHandle);

		BlitzenDX12::BeginRenderPassClear(cmd.m_graphicsCmdList.Get(), renderTarget, descriptorContext, pipelineContext, swapchainIDX);

		for (uint32_t ctx = 0; ctx < renderContextCount; ++ctx)
		{
			switch (renderContextArr[ctx].m_renderType)
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

		// Ends pass
		cmdList->EndRenderPass();
	}

	void SetupForFirstRenderPass(BlitzenDX12::Dx12Renderer* pRenderer)
	{
		UINT frame{ pRenderer->m_currentFrame };
		pRenderer->m_swapchainIDX = pRenderer->m_swapchain->GetCurrentBackBufferIndex();
		UINT swapchainIDX{ pRenderer->m_swapchainIDX };
		auto& cmd{ pRenderer->m_cmdContext[frame] };

		// Render target barrier
		D3D12_RESOURCE_BARRIER renderTargetBarrier{};
		BlitzenDX12::CreateResourcesTransitionBarrier(renderTargetBarrier, pRenderer->m_swapchainBackBuffers[swapchainIDX].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmd.m_graphicsCmdList->ResourceBarrier(1, &renderTargetBarrier);

		// Viewport and scissor
		BlitzenDX12::DefineViewportAndScissor(cmd.m_graphicsCmdList.Get(), (float)pRenderer->m_swapchainWidth, (float)pRenderer->m_swapchainHeight);
	}

	void FinalizeRendering(BlitzenDX12::Dx12Renderer* pRenderer)
	{
		auto& cmd{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };

		D3D12_RESOURCE_BARRIER presentBarrier{};
		BlitzenDX12::CreateResourcesTransitionBarrier(presentBarrier, pRenderer->m_swapchainBackBuffers[pRenderer->m_swapchainIDX].Get(), 
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cmd.m_graphicsCmdList->ResourceBarrier(1, &presentBarrier);

		cmd.m_graphicsCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmd.m_graphicsCmdList.Get() };
		pRenderer->m_commandQueue->ExecuteCommandLists(1, commandLists);
	}
}

#endif