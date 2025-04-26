#include "dx12Renderer.h"

namespace BlitzenDX12
{
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

		auto rtvHandle = m_swapchainRtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += m_currentFrame * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		frameTools.mainGraphicsCommandAllocator->Reset();
		frameTools.mainGraphicsCommandList->Reset(frameTools.mainGraphicsCommandAllocator.Get(), nullptr);

		frameTools.mainGraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		D3D12_RESOURCE_BARRIER attachmentBarrier{};
		attachmentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		attachmentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		attachmentBarrier.Transition.pResource = m_swapchainBackBuffers[m_currentFrame].Get();
		attachmentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		attachmentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		attachmentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		frameTools.mainGraphicsCommandList->ResourceBarrier(1, &attachmentBarrier);

		// Clear the render target to black or any color (optional)
		FLOAT clearColor[4] = { 0.f, 0.2f, 0.4f, 1.0f };
		frameTools.mainGraphicsCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		frameTools.mainGraphicsCommandList->SetGraphicsRootSignature(m_triangleRootSignature.Get());
		frameTools.mainGraphicsCommandList->SetPipelineState(m_trianglePso.Get());
		BlitML::vec3 triangleColor{ 0, 0.8f, 0.4f };
		//frameTools.mainGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 3, &triangleColor, 0);
		frameTools.mainGraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		frameTools.mainGraphicsCommandList->DrawInstanced(3, 1, 0, 0);

		D3D12_RESOURCE_BARRIER presentBarrier{};
		presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		presentBarrier.Transition.pResource = m_swapchainBackBuffers[m_currentFrame].Get();
		presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
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

		// Wait until the previous frame is finished.
		if (frameTools.inFlightFence->GetCompletedValue() < fence)
		{
			frameTools.inFlightFence->SetEventOnCompletion(fence, frameTools.inFlightFenceEvent);
			WaitForSingleObject(frameTools.inFlightFenceEvent, INFINITE);
		}

		m_currentFrame = m_swapchain->GetCurrentBackBufferIndex();
	}
}