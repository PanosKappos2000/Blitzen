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

	static void Present(Dx12Renderer::FrameTools& tools, IDXGISwapChain3* swapchain, ID3D12CommandQueue* commandQueue, ID3D12Resource* swapchainBackBuffer)
	{
		D3D12_RESOURCE_BARRIER presentBarrier{};
		CreateResourcesTransitionBarrier(presentBarrier, swapchainBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		tools.mainGraphicsCommandList->ResourceBarrier(1, &presentBarrier);

		tools.mainGraphicsCommandList->Close();
		ID3D12CommandList* commandLists[] = { tools.mainGraphicsCommandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		swapchain->Present(1, 0);
	}

	void Dx12Renderer::SetupWhileWaitingForPreviousFrame(const BlitzenEngine::DrawContext& context)
	{
		auto& frameTools = m_frameTools[m_currentFrame];

		const UINT64 fence = frameTools.inFlightFenceValue;
		m_commandQueue->Signal(frameTools.inFlightFence.Get(), fence);
		frameTools.inFlightFenceValue++;
		// Waits for the previous frame
		if (frameTools.inFlightFence->GetCompletedValue() < fence)
		{
			frameTools.inFlightFence->SetEventOnCompletion(fence, frameTools.inFlightFenceEvent);
			WaitForSingleObject(frameTools.inFlightFenceEvent, INFINITE);
		}
	}

	void Dx12Renderer::DrawFrame(BlitzenEngine::DrawContext& context)
	{
		auto& frameTools = m_frameTools[m_currentFrame];
		auto& varBuffers = m_varBuffers[m_currentFrame];
		const auto pCamera = context.pCamera;

		*m_varBuffers[m_currentFrame].viewDataBuffer.pData = pCamera->viewData;

		UINT swapchainIndex = m_swapchain->GetCurrentBackBufferIndex();
		frameTools.mainGraphicsCommandAllocator->Reset();
		frameTools.mainGraphicsCommandList->Reset(frameTools.mainGraphicsCommandAllocator.Get(), m_trianglePso.Get());

		// Binds heap for compute
		ID3D12DescriptorHeap* opaqueSrvHeaps[] = { m_srvHeap.Get() };
		frameTools.mainGraphicsCommandList->SetDescriptorHeaps(1, opaqueSrvHeaps);
		frameTools.mainGraphicsCommandList->SetComputeRootSignature(m_drawCountResetRoot.Get());

		// Compute heap handles
		auto sharedSrvHandle = m_descriptorContext.srvHandle;
		sharedSrvHandle.ptr += m_descriptorContext.sharedSrvOffset[m_currentFrame] * m_descriptorContext.srvIncrementSize;
		auto cullSrvHandle = m_descriptorContext.srvHandle;
		cullSrvHandle.ptr += m_descriptorContext.cullSrvOffset[m_currentFrame] * m_descriptorContext.srvIncrementSize;

		// Reource barrier before count is reset and commands are rewritten
		D3D12_RESOURCE_BARRIER preCullingDispatchBarriers[4]{};
		CreateResourcesTransitionBarrier(preCullingDispatchBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourcesTransitionBarrier(preCullingDispatchBarriers[1], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourceUAVBarrier(preCullingDispatchBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		CreateResourceUAVBarrier(preCullingDispatchBarriers[3], varBuffers.indirectDrawCount.buffer.Get());
		frameTools.mainGraphicsCommandList->ResourceBarrier(4, preCullingDispatchBarriers);

		// Resets count
		frameTools.mainGraphicsCommandList->SetPipelineState(m_drawCountResetPso.Get());
		frameTools.mainGraphicsCommandList->SetComputeRootDescriptorTable(0, cullSrvHandle);
		frameTools.mainGraphicsCommandList->Dispatch(1, 1, 1);

		D3D12_RESOURCE_BARRIER drawCountResetBarrier{};
		CreateResourceUAVBarrier(drawCountResetBarrier, varBuffers.indirectDrawCount.buffer.Get());
		frameTools.mainGraphicsCommandList->ResourceBarrier(1, &drawCountResetBarrier);

		// Cull pass
		frameTools.mainGraphicsCommandList->SetComputeRootSignature(m_drawCullSignature.Get());
		frameTools.mainGraphicsCommandList->SetComputeRootDescriptorTable(1, sharedSrvHandle);
		frameTools.mainGraphicsCommandList->SetComputeRootDescriptorTable(0, cullSrvHandle);
		frameTools.mainGraphicsCommandList->SetPipelineState(m_drawCull1Pso.Get());
		frameTools.mainGraphicsCommandList->SetComputeRoot32BitConstant(2, 1000/*context.pResources->renderObjectCount*/, 0);
		frameTools.mainGraphicsCommandList->Dispatch(BlitML::GetCompueShaderGroupSize(1000, 64), 1, 1);

		D3D12_RESOURCE_BARRIER postCullingBarriers[4]{};
		CreateResourcesTransitionBarrier(postCullingBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourcesTransitionBarrier(postCullingBarriers[1], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourceUAVBarrier(postCullingBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		CreateResourceUAVBarrier(postCullingBarriers[3], varBuffers.indirectDrawCount.buffer.Get());
		frameTools.mainGraphicsCommandList->ResourceBarrier(4, postCullingBarriers);

		frameTools.mainGraphicsCommandList->SetGraphicsRootSignature(m_opaqueRootSignature.Get());
		DefineViewportAndScissor(frameTools.mainGraphicsCommandList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

		// Render target barrier
		D3D12_RESOURCE_BARRIER attachmentBarriers[2]{};
		CreateResourcesTransitionBarrier(attachmentBarriers[0], m_swapchainBackBuffers[swapchainIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		frameTools.mainGraphicsCommandList->ResourceBarrier(1, attachmentBarriers);

		// Render target bind
		auto rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += (m_descriptorContext.swapchainRtvOffset + swapchainIndex) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		auto dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
		dsvHandle.ptr += (m_descriptorContext.depthTargetOffset + swapchainIndex) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		frameTools.mainGraphicsCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// Render target clear
		FLOAT clearColor[4] = { 0.f, 0.1f, 0.1f, 1.0f };
		frameTools.mainGraphicsCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		frameTools.mainGraphicsCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, Ce_ClearDepth, 0, 0, nullptr);

		// Opaque graphics pipeline exclusive descriptors
		auto opaqueSrvHandle = m_descriptorContext.srvHandle;
		opaqueSrvHandle.ptr += m_descriptorContext.opaqueSrvOffset[m_currentFrame] * m_descriptorContext.srvIncrementSize;
		frameTools.mainGraphicsCommandList->SetGraphicsRootDescriptorTable(0, opaqueSrvHandle);
		// Shared descriptors
		frameTools.mainGraphicsCommandList->SetGraphicsRootDescriptorTable(1, sharedSrvHandle);

		// Draws
		frameTools.mainGraphicsCommandList->SetPipelineState(m_opaqueGraphicsPso.Get());
		frameTools.mainGraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		frameTools.mainGraphicsCommandList->IASetIndexBuffer(&m_constBuffers.indexBufferView);
		UINT drawCmdOffset{ offsetof(IndirectDrawCmd, command) };

		frameTools.mainGraphicsCommandList->SetGraphicsRoot32BitConstant(2, 0, 0);
		frameTools.mainGraphicsCommandList->ExecuteIndirect(m_opaqueCmdSingature.Get(), 1000,
			varBuffers.indirectDrawBuffer.buffer.Get(), 0, varBuffers.indirectDrawCount.buffer.Get(), 0);

		Present(frameTools, m_swapchain.Get(), m_commandQueue.Get(), m_swapchainBackBuffers[swapchainIndex].Get());

		m_currentFrame = (m_currentFrame + 1) % ce_framesInFlight;
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

		auto rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += (m_descriptorContext.swapchainRtvOffset + m_currentFrame) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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