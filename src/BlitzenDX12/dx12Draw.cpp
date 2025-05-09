#include "dx12Renderer.h"
#include "dx12Resources.h"

namespace BlitzenDX12
{
	static void UpdateBuffers(Dx12Renderer::FrameTools& frameTools, Dx12Renderer::VarBuffers& varBuffers, 
		BlitzenEngine::Camera* pCamera, ID3D12CommandQueue* commandQueue)
	{
		*varBuffers.viewDataBuffer.pData = pCamera->viewData;

		frameTools.transferCommandAllocator->Reset();
		frameTools.transferCommandList->Reset(frameTools.transferCommandAllocator.Get(), nullptr);
		
		frameTools.transferCommandList->CopyBufferRegion(varBuffers.transformBuffer.buffer.Get(), 0, varBuffers.transformBuffer.staging.Get(), 0, varBuffers.transformBuffer.dataCopySize);
		
		frameTools.transferCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.transferCommandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);
		
		const UINT64 fence = frameTools.copyFenceValue++;
		commandQueue->Signal(frameTools.copyFence.Get(), fence);
		// Waits for the previous frame
		if (frameTools.copyFence->GetCompletedValue() < fence)
		{
			frameTools.copyFence->SetEventOnCompletion(fence, frameTools.copyFenceEvent);
			WaitForSingleObject(frameTools.copyFenceEvent, INFINITE);
		}
	}

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

	static void DrawCullPass(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap, D3D12_GPU_DESCRIPTOR_HANDLE sharedSrvHandle, 
		D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, Dx12Renderer::DescriptorContext& descriptorContext, UINT frame,
		ID3D12RootSignature* resetRoot, ID3D12PipelineState* resetPso, ID3D12RootSignature* cullRoot, ID3D12PipelineState* cullPso,
		Dx12Renderer::VarBuffers& varBuffers, uint32_t objCount)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { srvHeap };
		commandList->SetDescriptorHeaps(1, srvHeaps);
		commandList->SetComputeRootSignature(resetRoot);

		// Reource barrier before count is reset and commands are rewritten
		D3D12_RESOURCE_BARRIER preCullingDispatchBarriers[4]{};
		CreateResourcesTransitionBarrier(preCullingDispatchBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourcesTransitionBarrier(preCullingDispatchBarriers[1], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourceUAVBarrier(preCullingDispatchBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		CreateResourceUAVBarrier(preCullingDispatchBarriers[3], varBuffers.indirectDrawCount.buffer.Get());
		commandList->ResourceBarrier(4, preCullingDispatchBarriers);

		// Resets count
		commandList->SetPipelineState(resetPso);
		commandList->SetComputeRootDescriptorTable(0, cullSrvHandle);
		commandList->Dispatch(1, 1, 1);

		D3D12_RESOURCE_BARRIER drawCountResetBarrier{};
		CreateResourceUAVBarrier(drawCountResetBarrier, varBuffers.indirectDrawCount.buffer.Get());
		commandList->ResourceBarrier(1, &drawCountResetBarrier);

		// Culling
		commandList->SetComputeRootSignature(cullRoot);
		commandList->SetComputeRootDescriptorTable(1, sharedSrvHandle);
		commandList->SetComputeRootDescriptorTable(0, cullSrvHandle);
		commandList->SetPipelineState(cullPso);
		commandList->SetComputeRoot32BitConstant(2, objCount, 0);
		commandList->Dispatch(BlitML::GetCompueShaderGroupSize(objCount, 64), 1, 1);

		// Transitions and blocks graphics
		D3D12_RESOURCE_BARRIER postCullingBarriers[4]{};
		CreateResourcesTransitionBarrier(postCullingBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourcesTransitionBarrier(postCullingBarriers[1], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourceUAVBarrier(postCullingBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		CreateResourceUAVBarrier(postCullingBarriers[3], varBuffers.indirectDrawCount.buffer.Get());
		commandList->ResourceBarrier(4, postCullingBarriers);
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

	void Dx12Renderer::Update(const BlitzenEngine::DrawContext& context)
	{

	}

	void Dx12Renderer::UpdateObjectTransform(uint32_t trId, BlitzenEngine::MeshTransform& newTr)
	{
		auto pData = m_varBuffers[m_currentFrame].transformBuffer.pData;
		BlitzenCore::BlitMemCopy(reinterpret_cast<BlitzenEngine::MeshTransform*>(pData) + trId, &newTr, sizeof(BlitzenEngine::MeshTransform));
	}

	void Dx12Renderer::DrawFrame(BlitzenEngine::DrawContext& context)
	{
		auto& frameTools = m_frameTools[m_currentFrame];
		auto& varBuffers = m_varBuffers[m_currentFrame];
		const auto pCamera = context.pCamera;

		const UINT64 fence = frameTools.inFlightFenceValue;
		m_commandQueue->Signal(frameTools.inFlightFence.Get(), fence);
		frameTools.inFlightFenceValue++;
		// Waits for the previous frame
		if (frameTools.inFlightFence->GetCompletedValue() < fence)
		{
			frameTools.inFlightFence->SetEventOnCompletion(fence, frameTools.inFlightFenceEvent);
			WaitForSingleObject(frameTools.inFlightFenceEvent, INFINITE);
		}

		// Dynamic buffers
		UpdateBuffers(frameTools, varBuffers, context.pCamera, m_transferCommandQueue.Get());

		UINT swapchainIndex = m_swapchain->GetCurrentBackBufferIndex();
		frameTools.mainGraphicsCommandAllocator->Reset();
		frameTools.mainGraphicsCommandList->Reset(frameTools.mainGraphicsCommandAllocator.Get(), m_trianglePso.Get());

		D3D12_RESOURCE_BARRIER transformAfterCopyBarrier{};
		CreateResourcesTransitionBarrier(transformAfterCopyBarrier, varBuffers.transformBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		frameTools.mainGraphicsCommandList->ResourceBarrier(1, &transformAfterCopyBarrier);

		auto sharedSrvHandle = m_descriptorContext.srvHandle;
		sharedSrvHandle.ptr += m_descriptorContext.sharedSrvOffset[m_currentFrame] * m_descriptorContext.srvIncrementSize;
		auto cullSrvHandle = m_descriptorContext.srvHandle;
		cullSrvHandle.ptr += m_descriptorContext.cullSrvOffset[m_currentFrame] * m_descriptorContext.srvIncrementSize;

		// Frustum culling and lod selection, per object
		DrawCullPass(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), sharedSrvHandle, cullSrvHandle, m_descriptorContext,
			m_currentFrame, m_drawCountResetRoot.Get(), m_drawCountResetPso.Get(), m_drawCullSignature.Get(), m_drawCull1Pso.Get(), varBuffers,
			context.pResources->renderObjectCount);

		ID3D12DescriptorHeap* graphicsHeaps[] = { m_srvHeap.Get(), m_samplerHeap.Get()};
		frameTools.mainGraphicsCommandList->SetDescriptorHeaps(2, graphicsHeaps);
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
		frameTools.mainGraphicsCommandList->SetGraphicsRootDescriptorTable(Ce_OpaqueExclusiveBuffersElement, opaqueSrvHandle);
		frameTools.mainGraphicsCommandList->SetGraphicsRootDescriptorTable(Ce_OpaqueSharedBuffersElement, sharedSrvHandle);
		auto samplerSrvHandle = m_descriptorContext.samplerHandle;
		samplerSrvHandle.ptr += m_descriptorContext.defaultTextureSamplerOffset * m_descriptorContext.samplerIncrementSize;
		frameTools.mainGraphicsCommandList->SetGraphicsRootDescriptorTable(Ce_OpaqueSamplerElement, samplerSrvHandle);
		auto materialSrvHandle = m_descriptorContext.srvHandle;
		materialSrvHandle.ptr += m_descriptorContext.materialSrvOffset * m_descriptorContext.srvIncrementSize;
		frameTools.mainGraphicsCommandList->SetGraphicsRootDescriptorTable(Ce_MaterialSrvElement, materialSrvHandle);

		// Draw
		frameTools.mainGraphicsCommandList->SetGraphicsRoot32BitConstant(Ce_TextureDescriptorsOffsetElement, (UINT)m_descriptorContext.texturesSrvOffset, 0);
		frameTools.mainGraphicsCommandList->SetPipelineState(m_opaqueGraphicsPso.Get());
		frameTools.mainGraphicsCommandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		frameTools.mainGraphicsCommandList->IASetIndexBuffer(&m_constBuffers.indexBufferView);
		frameTools.mainGraphicsCommandList->ExecuteIndirect(m_opaqueCmdSingature.Get(), Ce_IndirectDrawCmdBufferSize,
			varBuffers.indirectDrawBuffer.buffer.Get(), 0, varBuffers.indirectDrawCount.buffer.Get(), 0);

		D3D12_RESOURCE_BARRIER transformCopyBarrier{};
		CreateResourcesTransitionBarrier(transformCopyBarrier, varBuffers.transformBuffer.buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, 
			D3D12_RESOURCE_STATE_COPY_DEST);
		frameTools.mainGraphicsCommandList->ResourceBarrier(1, &transformCopyBarrier);

		Present(frameTools, m_swapchain.Get(), m_commandQueue.Get(), m_swapchainBackBuffers[swapchainIndex].Get());

		m_currentFrame = (m_currentFrame + 1) % ce_framesInFlight;
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