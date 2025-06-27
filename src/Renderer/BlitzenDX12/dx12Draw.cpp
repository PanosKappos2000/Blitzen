#if defined(_WIN32)
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenDX12
{
	void Dx12Renderer::DrawWhileWaiting(float deltaTime)
	{
		auto& cmdContext = m_cmdContext[m_currentFrame];

		PlaceFence(cmdContext.m_frameFence.m_value, m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);

		m_swapchainIDX = m_swapchain->GetCurrentBackBufferIndex();

		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), m_pipelineContext.m_trianglePso.Get());

		cmdContext.m_graphicsCmdList->SetGraphicsRootSignature(m_pipelineContext.m_triangleRoot.Get());
		DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

		D3D12_RESOURCE_BARRIER attachmentBarrier{};
		CreateResourcesTransitionBarrier(attachmentBarrier, m_swapchainBackBuffers[m_swapchainIDX].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &attachmentBarrier);

		cmdContext.m_graphicsCmdList->OMSetRenderTargets(1, &m_descriptorContext.m_swapchainRtvHandle[m_currentFrame], FALSE, nullptr);

		cmdContext.m_graphicsCmdList->ClearRenderTargetView(m_descriptorContext.m_swapchainRtvHandle[m_currentFrame], BlitzenCore::Ce_DefaultWindowBackgroundColor, 0, nullptr);

		//BlitML::vec3 triangleColor{ 0, 0.8f, 0.4f };
		//frameTools.mainGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 3, &triangleColor, 0);
		cmdContext.m_graphicsCmdList->SetPipelineState(m_pipelineContext.m_trianglePso.Get());
		cmdContext.m_graphicsCmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdContext.m_graphicsCmdList->DrawInstanced(3, 1, 0, 0);

		D3D12_RESOURCE_BARRIER presentBarrier{};
		CreateResourcesTransitionBarrier(presentBarrier, m_swapchainBackBuffers[m_swapchainIDX].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &presentBarrier);

		cmdContext.m_graphicsCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_graphicsCmdList.Get() };
		m_commandQueue->ExecuteCommandLists(1, commandLists);
	}
}

#endif