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
		
		frameTools.transferCommandList->CopyBufferRegion(varBuffers.transformBuffer.buffer.Get(), 0, varBuffers.transformBuffer.staging.Get(), 
			0, varBuffers.transformBuffer.dataCopySize);
		
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

	static void RecreateSwapchain(IDXGIFactory6* factory, ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t newWidth, uint32_t newHeight,
		DX12WRAPPER<IDXGISwapChain3>* pSwapchain, DX12WRAPPER<ID3D12Resource>* pSwapchainBuffers, DX12WRAPPER<ID3D12Resource>* pDepthTargets, 
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle, Dx12Renderer::FrameTools* pTools)
	{
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& tools{ pTools[i] };
			PlaceFence(tools.inFlightFenceValue, queue, tools.inFlightFence.Get(), tools.inFlightFenceEvent);

			pSwapchainBuffers[i]->Release();
			pDepthTargets[i]->Release();
		}

		pSwapchain->ReleaseAndGetAddressOf();
		auto hwnd = reinterpret_cast<HWND>(BlitzenPlatform::GetWindowHandle());
		BLIT_ASSERT(CreateSwapchain(factory, queue, newWidth, newHeight, hwnd, pSwapchain));
		
		// Automatically releases when trying to create. Creates 
		SIZE_T rtvSwapchainOffset{ 0 };
		BLIT_ASSERT(CreateSwapchainResources(pSwapchain->Get(), device, pSwapchainBuffers, rtvHeapHandle, rtvSwapchainOffset));
		
		SIZE_T dsvDepthTargetOffset{ 0 };
		BLIT_ASSERT(CreateDepthTargets(device, pDepthTargets, dsvHeapHandle, dsvDepthTargetOffset, newWidth, newHeight));
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
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, cullSrvHandle);
		commandList->Dispatch(1, 1, 1);

		D3D12_RESOURCE_BARRIER drawCountResetBarrier{};
		CreateResourceUAVBarrier(drawCountResetBarrier, varBuffers.indirectDrawCount.buffer.Get());
		commandList->ResourceBarrier(1, &drawCountResetBarrier);

		// Culling
		commandList->SetComputeRootSignature(cullRoot);
		commandList->SetComputeRootDescriptorTable(Ce_CullSharedSRVsParameterId, sharedSrvHandle);
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, cullSrvHandle);
		commandList->SetPipelineState(cullPso);
		commandList->SetComputeRoot32BitConstant(Ce_CullDrawCountParameterId, objCount, 0);
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

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

	static void DrawInstanceCullPass(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap, D3D12_GPU_DESCRIPTOR_HANDLE sharedSrvHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, Dx12Renderer::DescriptorContext& descriptorContext, UINT frame,
		ID3D12RootSignature* resetRoot, ID3D12PipelineState* resetPso, ID3D12RootSignature* cullRoot, ID3D12PipelineState* instResetPso, 
		ID3D12PipelineState* cullPso, ID3D12PipelineState* drawInstCmdPso, Dx12Renderer::VarBuffers& varBuffers, BlitzenEngine::RenderingResources* pResources)
	{
		size_t lodDataCount{ pResources->GetLodData().GetSize() };
		uint32_t objCount{ pResources->renderObjectCount };

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

		// Barrier for inst count reset
		D3D12_RESOURCE_BARRIER resetInstBarrier{};
		CreateResourceUAVBarrier(resetInstBarrier, varBuffers.lodInstBuffer.buffer.Get());
		commandList->ResourceBarrier(1, &resetInstBarrier);

		commandList->SetComputeRootSignature(cullRoot);

		commandList->SetPipelineState(instResetPso);
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, cullSrvHandle);
		commandList->SetComputeRoot32BitConstant(Ce_CullDrawCountParameterId, (UINT)lodDataCount, 0);
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(uint32_t(lodDataCount), 64), 1, 1);

		// Wait for reset before culling
		D3D12_RESOURCE_BARRIER countResetBarriers[3]{};
		CreateResourceUAVBarrier(countResetBarriers[0], varBuffers.indirectDrawCount.buffer.Get());
		CreateResourceUAVBarrier(countResetBarriers[1], varBuffers.lodInstBuffer.buffer.Get());
		CreateResourceUAVBarrier(countResetBarriers[2], varBuffers.drawInstBuffer.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(countResetBarriers), countResetBarriers);

		// Culling
		commandList->SetComputeRootDescriptorTable(Ce_CullSharedSRVsParameterId, sharedSrvHandle);
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, cullSrvHandle);
		commandList->SetPipelineState(cullPso);
		commandList->SetComputeRoot32BitConstant(Ce_CullDrawCountParameterId, objCount, 0);
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Waits for culling and instance count
		D3D12_RESOURCE_BARRIER instancingBarrier{};
		CreateResourceUAVBarrier(instancingBarrier, varBuffers.lodInstBuffer.buffer.Get());
		commandList->ResourceBarrier(1, &instancingBarrier);

		// Creates commands with instancing
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, cullSrvHandle);
		commandList->SetPipelineState(drawInstCmdPso);
		commandList->SetComputeRoot32BitConstant(Ce_CullDrawCountParameterId, (UINT)lodDataCount, 0);
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize((uint32_t)lodDataCount, 64), 1, 1);

		// Transitions and blocks graphics
		D3D12_RESOURCE_BARRIER postCullingBarriers[5]{};
		CreateResourcesTransitionBarrier(postCullingBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourcesTransitionBarrier(postCullingBarriers[1], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourceUAVBarrier(postCullingBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		CreateResourceUAVBarrier(postCullingBarriers[3], varBuffers.indirectDrawCount.buffer.Get());
		CreateResourceUAVBarrier(postCullingBarriers[4], varBuffers.drawInstBuffer.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(postCullingBarriers), postCullingBarriers);
	}

	static void ClearWindow(ID3D12GraphicsCommandList* cmdList, float swapchainWidth, float swapchainHeight, 
		ID3D12Resource* swapchainBackBuffer, ID3D12DescriptorHeap* rvtHeap, ID3D12DescriptorHeap* dsvHeap, 
		Dx12Renderer::DescriptorContext& descriptorContext, UINT swapchainIndex)
	{
		DefineViewportAndScissor(cmdList, swapchainWidth, swapchainHeight);

		// Render target barrier
		D3D12_RESOURCE_BARRIER attachmentBarriers[2]{};
		CreateResourcesTransitionBarrier(attachmentBarriers[0], swapchainBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdList->ResourceBarrier(1, attachmentBarriers);

		// Render target bind
		auto rtvHandle = rvtHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += (descriptorContext.swapchainRtvOffset + swapchainIndex) * descriptorContext.rtvIncrementSize;
		auto dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		dsvHandle.ptr += (descriptorContext.depthTargetOffset + swapchainIndex) * descriptorContext.dsvIncrementSize;
		cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		// Render target clear
		cmdList->ClearRenderTargetView(rtvHandle, BlitzenEngine::Ce_DefaultWindowBackgroundColor, 0, nullptr);
		cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, Ce_ClearDepth, 0, 0, nullptr);
	}

	static void DrawIndirect(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap, ID3D12DescriptorHeap* samplerHeap,
		ID3D12RootSignature* rootSig, ID3D12PipelineState* pipeline, ID3D12CommandSignature* cmdSig, Dx12Renderer::DescriptorContext& context, 
		Dx12Renderer::ConstBuffers& constBuffers, Dx12Renderer::VarBuffers& varBuffers, UINT currentFrame)
	{
		ID3D12DescriptorHeap* graphicsHeaps[] = { srvHeap, samplerHeap};
		commandList->SetDescriptorHeaps(2, graphicsHeaps);
		commandList->SetGraphicsRootSignature(rootSig);

		// Opaque graphics pipeline exclusive descriptors
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueExclusiveBuffersElement, context.opaqueSrvHandle[currentFrame]);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueSharedBuffersElement, context.sharedSrvHandle[currentFrame]);
		commandList->SetGraphicsRootDescriptorTable(Ce_MaterialSrvElement, context.materialSrvHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_TextureDescriptorsElement, context.textureSrvHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueSamplerElement, context.samplerHandle);

		// Draw
		commandList->SetPipelineState(pipeline);
		commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetIndexBuffer(&constBuffers.indexBufferView);
		commandList->ExecuteIndirect(cmdSig, Ce_IndirectDrawCmdBufferSize, varBuffers.indirectDrawBuffer.buffer.Get(), 0, varBuffers.indirectDrawCount.buffer.Get(), 0);
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

		if (context.pCamera->transformData.bWindowResize)
		{
			m_swapchainWidth = (UINT)context.pCamera->transformData.windowWidth;
			m_swapchainHeight = (UINT)context.pCamera->transformData.windowHeight;
			RecreateSwapchain(m_factory.Get(), m_device.Get(), m_commandQueue.Get(), m_swapchainWidth, m_swapchainHeight, &m_swapchain, 
				m_swapchainBackBuffers, m_depthBuffers, m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), 
				m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameTools.Data());
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

		if constexpr (Ce_DrawOcclusion)
		{
			ClearWindow(frameTools.mainGraphicsCommandList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight,
				m_swapchainBackBuffers[swapchainIndex].Get(), m_rtvHeap.Get(), m_dsvHeap.Get(), m_descriptorContext, swapchainIndex);
		}
		else if constexpr (BlitzenEngine::Ce_InstanceCulling)
		{
			DrawInstanceCullPass(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), m_descriptorContext.sharedSrvHandle[m_currentFrame], 
				m_descriptorContext.cullSrvHandle[m_currentFrame], m_descriptorContext, m_currentFrame, m_drawCountResetRoot.Get(), 
				m_drawCountResetPso.Get(), m_drawCullSignature.Get(), m_drawInstCountResetPso.Get(), m_drawCullPso.Get(), m_drawInstCmdPso.Get(), 
				varBuffers, context.pResources);

			ClearWindow(frameTools.mainGraphicsCommandList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight, 
				m_swapchainBackBuffers[swapchainIndex].Get(), m_rtvHeap.Get(), m_dsvHeap.Get(), m_descriptorContext, swapchainIndex);

			DrawIndirect(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), m_samplerHeap.Get(), m_opaqueRootSignature.Get(),
				m_opaqueGraphicsPso.Get(), m_opaqueCmdSingature.Get(), m_descriptorContext, m_constBuffers, varBuffers, m_currentFrame);
		}
		else
		{
			// Frustum culling and lod selection, per object
			DrawCullPass(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), m_descriptorContext.sharedSrvHandle[m_currentFrame],
				m_descriptorContext.cullSrvHandle[m_currentFrame], m_descriptorContext, m_currentFrame, m_drawCountResetRoot.Get(), m_drawCountResetPso.Get(),
				m_drawCullSignature.Get(), m_drawCullPso.Get(), varBuffers, context.pResources->renderObjectCount);

			ClearWindow(frameTools.mainGraphicsCommandList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight,
				m_swapchainBackBuffers[swapchainIndex].Get(), m_rtvHeap.Get(), m_dsvHeap.Get(), m_descriptorContext, swapchainIndex);

			DrawIndirect(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), m_samplerHeap.Get(), m_opaqueRootSignature.Get(),
				m_opaqueGraphicsPso.Get(), m_opaqueCmdSingature.Get(), m_descriptorContext, m_constBuffers, varBuffers, m_currentFrame);
		}

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

		frameTools.mainGraphicsCommandList->ClearRenderTargetView(rtvHandle, BlitzenEngine::Ce_DefaultWindowBackgroundColor, 0, nullptr);

		//BlitML::vec3 triangleColor{ 0, 0.8f, 0.4f };
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