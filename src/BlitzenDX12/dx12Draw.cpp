#include "dx12Renderer.h"
#include "dx12Resources.h"

namespace BlitzenDX12
{
	static void DefineViewportAndScissor(ID3D12GraphicsCommandList* commandList, float width, float height)
	{
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
	}

	void Dx12Renderer::SetupWhileWaitingForPreviousFrame(const BlitzenEngine::DrawContext& context)
	{
		
	}

    void Dx12Renderer::DrawFrame(BlitzenEngine::DrawContext& context)
    {

    }

	void Dx12Renderer::UpdateObjectTransform(uint32_t trId, BlitzenEngine::MeshTransform& newTr)
	{
		
	}

	void Dx12Renderer::DrawWhileWaiting()
	{
		auto& frameTools = m_frameTools[m_currentFrame];

		UINT swapchainIndex = m_swapchain->GetCurrentBackBufferIndex();

		frameTools.mainGraphicsCommandAllocator->Reset();
		frameTools.mainGraphicsCommandList->Reset(frameTools.mainGraphicsCommandAllocator.Get(), m_trianglePso.Get());

		frameTools.mainGraphicsCommandList->SetGraphicsRootSignature(m_triangleRootSignature.Get());
		DefineViewportAndScissor(frameTools.mainGraphicsCommandList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

		D3D12_RESOURCE_BARRIER attachmentBarrier{};
		CreateResourcesTransitionBarrier(attachmentBarrier, m_swapchainBackBuffers[swapchainIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		frameTools.mainGraphicsCommandList->ResourceBarrier(1, &attachmentBarrier);

		auto rtvHandle = m_swapchainRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += swapchainIndex * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		frameTools.mainGraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		FLOAT clearColor[4] = { 0.f, 0.2f, 0.4f, 1.0f };
		frameTools.mainGraphicsCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		BlitML::vec3 triangleColor{ 0, 0.8f, 0.4f };
		//frameTools.mainGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 3, &triangleColor, 0);
		frameTools.mainGraphicsCommandList->SetPipelineState(m_trianglePso.Get());
		frameTools.mainGraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		frameTools.mainGraphicsCommandList->DrawInstanced(3, 1, 0, 0);

		D3D12_RESOURCE_BARRIER presentBarrier{};
		CreateResourcesTransitionBarrier(presentBarrier, m_swapchainBackBuffers[swapchainIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		frameTools.mainGraphicsCommandList->ResourceBarrier(1, &presentBarrier);

		frameTools.mainGraphicsCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.mainGraphicsCommandList.Get() };
		m_commandQueue->ExecuteCommandLists(1, commandLists);

		auto result = m_swapchain->Present(1, 0);
		if (FAILED(result))
		{
			LOG_ERROR_MESSAGE_AND_RETURN(result);
		}

		const UINT64 fence = frameTools.inFlightFenceValue;
		m_commandQueue->Signal(frameTools.inFlightFence.Get(), fence);
		frameTools.inFlightFenceValue++;
		// Waits for the previous frame
		if (frameTools.inFlightFence->GetCompletedValue() < fence)
		{
			frameTools.inFlightFence->SetEventOnCompletion(fence, frameTools.inFlightFenceEvent);
			WaitForSingleObject(frameTools.inFlightFenceEvent, INFINITE);
		}

		m_currentFrame = (m_currentFrame + 1) % ce_framesInFlight;
	}
}