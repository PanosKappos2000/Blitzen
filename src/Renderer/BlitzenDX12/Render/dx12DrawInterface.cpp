#if defined(_WIN32)
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"

namespace BlitzenEngine
{
	void RenderObjects(BlitzenDX12::Dx12Renderer* pRenderer, const RENDER_CONTEXT& renderContext)
	{
		UINT frame{ pRenderer->m_currentFrame };
		UINT swapchainIDX{ pRenderer->m_swapchainIDX };
		auto& cmd{ pRenderer->m_cmdContext[frame] };
		auto cmdList{ cmd.m_graphicsCmdList };
		auto renderTarget{ pRenderer->m_swapchainBackBuffers[swapchainIDX].Get()};
		auto& descriptorContext{ pRenderer->m_descriptorContext };
		auto& pipelineContext{ pRenderer->m_pipelineContext };
		auto& rwResources{ pRenderer->m_rwResources[frame] };

		switch (renderContext.m_renderType)
		{
		case BLIT_RENDER_TYPE::RENDER_OPAQUE:
		{
			BlitzenDX12::BeginRenderPassClear(cmd.m_graphicsCmdList.Get(), renderTarget, descriptorContext, pipelineContext, swapchainIDX);

			ID3D12DescriptorHeap* graphicsHeaps[] = { descriptorContext.m_viewHeap.Get(), descriptorContext.m_samplerHeap.Get() };
			cmdList->SetDescriptorHeaps(2, graphicsHeaps);
			cmdList->SetGraphicsRootSignature(pipelineContext.m_opaqueDrawRoot.Get());

			// DESCRIPTORS
			cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::Ce_OpaqueDrawExclusiveSRVsRootID, descriptorContext.m_opaqueDrawViewsExclusiveHandle[frame]);
			cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::Ce_OpaqueDrawSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
			cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::Ce_OpaqueDrawTexSMPRootID, descriptorContext.m_texSmpHandle);
			cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::Ce_OpaqueDrawMatSRVRootID, descriptorContext.m_opaqueDrawPSExclusiveViewsHandle);
			cmdList->SetGraphicsRootDescriptorTable(BlitzenDX12::Ce_OpaqueDrawTexSRVRootID, descriptorContext.m_texDescriptorsSRVHandle);

			// Pipeline
			cmdList->SetPipelineState(pipelineContext.m_opaqueDrawPso.Get());
			// Primitives
			cmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			// Index buffer
			cmdList->IASetIndexBuffer(&pRenderer->m_roResources.m_idxBuffer.m_view);
			// DRAW
			cmdList->ExecuteIndirect(pipelineContext.m_opaqueDrawCmdSign.Get(), BlitzenDX12::Ce_IndirectDrawCmdBufferSize, rwResources.m_drawCmdBuffer.buffer.Get(), 0,
				rwResources.m_drawCmdCounterBuffer.buffer.Get(), 0);

			// Ends pass
			cmdList->EndRenderPass();

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