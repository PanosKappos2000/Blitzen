#include "dx12Renderer.h"
#include "dx12RNDResources.h"

namespace BlitzenDX12
{
	static void UpdateBuffers(Dx12Renderer::FrameTools& frameTools, Dx12Renderer::VarBuffers& varBuffers, 
		BlitzenEngine::Camera* pCamera, ID3D12CommandQueue* commandQueue)
	{
		if (pCamera->transformData.bFreezeFrustum)
		{
			// Only change the matrix that moves the camera if the freeze frustum debug functionality is active
			varBuffers.viewDataBuffer.pData->projectionViewMatrix = pCamera->viewData.projectionViewMatrix;
		}
		else
		{
			*varBuffers.viewDataBuffer.pData = pCamera->viewData;
		}

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

	static void RecreateDepthPyramidDescriptors(ID3D12Device* device, Dx12Renderer::VarBuffers* pBuffers, Dx12Renderer::DescriptorContext& context,
		ID3D12DescriptorHeap* srvHeap, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight)
	{
		{
			for (uint32_t i = 0; i < ce_framesInFlight; ++i)
			{
				auto& var = pBuffers[i];

				CreateTextureShaderResourceView(device, var.depthPyramid.pyramid.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
					context.depthPyramidSrvOffset[i], Ce_DepthPyramidFormat, var.depthPyramid.mipCount);
			}

			for (uint32_t f = 0; f < ce_framesInFlight; ++f)
			{
				auto& var = pBuffers[f];

				for (uint32_t i = 0; i < var.depthPyramid.mipCount; ++i)
				{
					Create2DTextureUnorderedAccessView(device, var.depthPyramid.pyramid.Get(), Ce_DepthPyramidFormat, i, context.depthPyramidSrvOffset[f],
						srvHeap->GetCPUDescriptorHandleForHeapStart());
				}
			}

			for (size_t i = 0; i < ce_framesInFlight; ++i)
			{
				CreateTextureShaderResourceView(device, pDepthTargets[i].Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
					context.depthPyramidSrvOffset[i], Ce_DepthTargetFormat, 1);
			}
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

	static void DrawCountReset(ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* resetRoot, 
		ID3D12PipelineState* resetPso, D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, Dx12Renderer::VarBuffers& varBuffers)
	{
		// Reource barrier before count is reset and commands are rewritten
		D3D12_RESOURCE_BARRIER resetBarriers[2]{};
		CreateResourcesTransitionBarrier(resetBarriers[0], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourceUAVBarrier(resetBarriers[1], varBuffers.indirectDrawCount.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(resetBarriers), resetBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(resetRoot);
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, cullSrvHandle);

		// Pipeline + constants
		commandList->SetPipelineState(resetPso);

		commandList->Dispatch(1, 1, 1);
	}

	static void CullObjects(ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* cullRoot, ID3D12PipelineState* cullPso, uint32_t objCount, 
		Dx12Renderer::DescriptorContext& descriptorContext, UINT frame)
	{
		// Descriptors
		commandList->SetComputeRootSignature(cullRoot);
		commandList->SetComputeRootDescriptorTable(Ce_CullSharedSRVsParameterId, descriptorContext.sharedSrvHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, descriptorContext.cullSrvHandle[frame]);

		// Pipeline + constants
		commandList->SetPipelineState(cullPso);
		commandList->SetComputeRoot32BitConstant(Ce_CullDrawCountParameterId, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);
	}

	static void DrawCullPass(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap, D3D12_GPU_DESCRIPTOR_HANDLE sharedSrvHandle, 
		D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, Dx12Renderer::DescriptorContext& descriptorContext, UINT frame,
		ID3D12RootSignature* resetRoot, ID3D12PipelineState* resetPso, ID3D12RootSignature* cullRoot, ID3D12PipelineState* cullPso,
		Dx12Renderer::VarBuffers& varBuffers, uint32_t objCount)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { srvHeap };
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, resetRoot, resetPso, cullSrvHandle, varBuffers);

		// Culling barrier, waits for draw count reset and draw command read
		D3D12_RESOURCE_BARRIER cullingBarriers[3]{};
		// Count reset barrier 
		CreateResourceUAVBarrier(cullingBarriers[0], varBuffers.indirectDrawCount.buffer.Get());
		// Command read barrier
		CreateResourcesTransitionBarrier(cullingBarriers[1], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourceUAVBarrier(cullingBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Cull
		CullObjects(commandList, cullRoot, cullPso, objCount, descriptorContext, frame);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[4]{};
		CreateResourcesTransitionBarrier(graphicsBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourcesTransitionBarrier(graphicsBarriers[1], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourceUAVBarrier(graphicsBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		CreateResourceUAVBarrier(graphicsBarriers[3], varBuffers.indirectDrawCount.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void DrawOccFirstPass(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap, D3D12_GPU_DESCRIPTOR_HANDLE sharedSrvHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, Dx12Renderer::DescriptorContext& descriptorContext, UINT frame,
		ID3D12RootSignature* resetRoot, ID3D12PipelineState* resetPso, ID3D12RootSignature* cullRoot, ID3D12PipelineState* cullPso,
		Dx12Renderer::VarBuffers& varBuffers, uint32_t objCount)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { srvHeap };
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, resetRoot, resetPso, cullSrvHandle, varBuffers);

		// Culling barrier, waits for draw count reset and draw command read, and draw visibility write as well
		D3D12_RESOURCE_BARRIER cullingBarriers[4]{};
		// Count reset barrier 
		CreateResourceUAVBarrier(cullingBarriers[0], varBuffers.indirectDrawCount.buffer.Get());
		// Command read barrier
		CreateResourcesTransitionBarrier(cullingBarriers[1], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourceUAVBarrier(cullingBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		// Draw visibility barrier
		CreateResourceUAVBarrier(cullingBarriers[3], varBuffers.drawVisibilityBuffer.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// CULL PREVIOUS FRAME VISIBLE
		CullObjects(commandList, cullRoot, cullPso, objCount, descriptorContext, frame);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[4]{};
		CreateResourcesTransitionBarrier(graphicsBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourcesTransitionBarrier(graphicsBarriers[1], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourceUAVBarrier(graphicsBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		CreateResourceUAVBarrier(graphicsBarriers[3], varBuffers.indirectDrawCount.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void GenerateDepthPyramid(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap, Dx12Renderer::DescriptorContext& descriptorContext,
		UINT frame, UINT swapchainId, ID3D12RootSignature* pyramidSig, ID3D12PipelineState* pyramidPso, Dx12Renderer::VarBuffers& varBuffers, ID3D12Resource* depthTarget, 
		ID3D12DescriptorHeap* samplerHeap, uint32_t swapchainWidth, uint32_t swapchainHeight)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* heaps[] = { srvHeap, samplerHeap };
		commandList->SetDescriptorHeaps(2, heaps);

		// Barrier for depth pyramid generation, waits for depth target write and depth pyramid read
		D3D12_RESOURCE_BARRIER depthPyramidBarriers[2]{};
		CreateResourcesTransitionBarrier(depthPyramidBarriers[0], depthTarget,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		// Depth pyramid mips
		CreateResourcesTransitionBarrier(depthPyramidBarriers[1], varBuffers.depthPyramid.pyramid.Get(),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// Execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(depthPyramidBarriers), depthPyramidBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pyramidSig);
		commandList->SetPipelineState(pyramidPso);
		commandList->SetComputeRootDescriptorTable(Ce_DepthPyramidSamplerParameterId, descriptorContext.depthPyramidSamplerHandle);

		DepthPyramidRootConstant depthPyramidConstants{ BlitML::vec2{float(swapchainWidth), float(swapchainHeight)}, 0 };

		for (uint32_t i = 0; i < varBuffers.depthPyramid.mipCount; ++i)
		{
			// Mip size calculcations
			uint32_t levelWidth = BlitML::Max(1u, (varBuffers.depthPyramid.width) >> i);
			uint32_t levelHeight = BlitML::Max(1u, (varBuffers.depthPyramid.height) >> i);

			depthPyramidConstants.pyramidSize = BlitML::vec2{ float(levelWidth), float(levelHeight) };
			commandList->SetComputeRoot32BitConstants(Ce_DepthPyramidRootConstantParameterId, Ce_DepthPyramidRootConstantsCount, &depthPyramidConstants, 0);

			// Binds write texture (the depth pyramid has a copy for double buffering and each one has the correct offsets for the descriptor heap)
			commandList->SetComputeRootDescriptorTable(Ce_DepthPyramidUAVRootParameterId, varBuffers.depthPyramid.mips[i]);

			// Binds read texture (For first level, it's the depth target. For every other level, it's the depth pyramid itself)
			if (i == 0)
			{
				commandList->SetComputeRootDescriptorTable(Ce_DepthPyramidSRVRootParameterId, descriptorContext.depthTargetSrvHandle[swapchainId]);
			}
			else
			{
				commandList->SetComputeRootDescriptorTable(Ce_DepthPyramidSRVRootParameterId, descriptorContext.depthPyramidSrvHandle[frame]);
				depthPyramidConstants.level++;
			}

			// Generate level
			commandList->Dispatch(BlitML::GetComputeShaderGroupSize(levelWidth, 32), BlitML::GetComputeShaderGroupSize(levelHeight, 32), 1);

			// Barrier for the next loop, since it will use the current mip as the read descriptor
			D3D12_RESOURCE_BARRIER nextLoopBarrier{};
			CreateResourceUAVBarrier(nextLoopBarrier, varBuffers.depthPyramid.pyramid.Get());
			commandList->ResourceBarrier(1, &nextLoopBarrier);
		}

		D3D12_RESOURCE_BARRIER cullingBarrier{};
		CreateResourcesTransitionBarrier(cullingBarrier, varBuffers.depthPyramid.pyramid.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &cullingBarrier);

		D3D12_RESOURCE_BARRIER graphicsBarrier{};
		CreateResourcesTransitionBarrier(graphicsBarrier, depthTarget,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandList->ResourceBarrier(1, &graphicsBarrier);
	}

	static void DrawOccLatePass(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* srvHeap, D3D12_GPU_DESCRIPTOR_HANDLE sharedSrvHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, Dx12Renderer::DescriptorContext& descriptorContext, UINT frame,
		ID3D12RootSignature* resetRoot, ID3D12PipelineState* resetPso, ID3D12RootSignature* cullRoot, ID3D12PipelineState* cullPso,
		Dx12Renderer::VarBuffers& varBuffers, uint32_t objCount, ID3D12DescriptorHeap* samplerHeap)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { srvHeap, samplerHeap };
		commandList->SetDescriptorHeaps(2, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, resetRoot, resetPso, cullSrvHandle, varBuffers);

		// Culling barrier, waits for draw count reset and draw command read, and draw visibility read as well
		// Finally, it needs to wait for the depth pyramid (occlusion culling)
		D3D12_RESOURCE_BARRIER cullingBarriers[4]{};
		// Count reset barrier 
		CreateResourceUAVBarrier(cullingBarriers[0], varBuffers.indirectDrawCount.buffer.Get());
		// Command read barrier
		CreateResourcesTransitionBarrier(cullingBarriers[1], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourceUAVBarrier(cullingBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		// Draw visibility barrier
		CreateResourceUAVBarrier(cullingBarriers[3], varBuffers.drawVisibilityBuffer.buffer.Get());
		// Depth pyramid barrier
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(cullRoot);
		commandList->SetComputeRootDescriptorTable(Ce_CullSharedSRVsParameterId, descriptorContext.sharedSrvHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, descriptorContext.cullSrvHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccLateDepthPyramidParameterId, descriptorContext.depthPyramidSrvHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccLateDepthPyramidSamplerParameterId, descriptorContext.depthPyramidSamplerHandle);

		// Pipeline + constants
		commandList->SetPipelineState(cullPso);
		commandList->SetComputeRoot32BitConstant(Ce_CullDrawCountParameterId, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[4]{};
		CreateResourcesTransitionBarrier(graphicsBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourcesTransitionBarrier(graphicsBarriers[1], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourceUAVBarrier(graphicsBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		CreateResourceUAVBarrier(graphicsBarriers[3], varBuffers.indirectDrawCount.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
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

		// Resets Count
		DrawCountReset(commandList, resetRoot, resetPso, cullSrvHandle, varBuffers);

		// Blocks instance counter reset
		D3D12_RESOURCE_BARRIER instCounterResetBarrier{};
		CreateResourceUAVBarrier(instCounterResetBarrier, varBuffers.lodInstBuffer.buffer.Get());
		commandList->ResourceBarrier(1, &instCounterResetBarrier);

		// Descriptors
		commandList->SetComputeRootSignature(cullRoot);
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, cullSrvHandle);

		// Pipeline + Constants
		commandList->SetPipelineState(instResetPso);
		commandList->SetComputeRoot32BitConstant(Ce_CullDrawCountParameterId, (UINT)lodDataCount, 0);

		// Resets instance counters
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(uint32_t(lodDataCount), 64), 1, 1);

		// Culling barriers. Waits for draw and instance count reset, instance buffer and draw buffer read
		D3D12_RESOURCE_BARRIER cullingBarriers[5]{};
		// Draw commands barrier
		CreateResourcesTransitionBarrier(cullingBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourceUAVBarrier(cullingBarriers[1], varBuffers.indirectDrawBuffer.buffer.Get());
		// Draw count barrier
		CreateResourceUAVBarrier(cullingBarriers[2], varBuffers.indirectDrawCount.buffer.Get());
		// Instance Counter barrier
		CreateResourceUAVBarrier(cullingBarriers[3], varBuffers.lodInstBuffer.buffer.Get());
		// Instance barrier
		CreateResourceUAVBarrier(cullingBarriers[4], varBuffers.drawInstBuffer.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Cull
		CullObjects(commandList, cullRoot, cullPso, objCount, descriptorContext, frame);

		// Blocks instancing until the instance counters are set
		D3D12_RESOURCE_BARRIER instancingBarrier{};
		CreateResourceUAVBarrier(instancingBarrier, varBuffers.lodInstBuffer.buffer.Get());
		commandList->ResourceBarrier(1, &instancingBarrier);

		// Descriptors
		commandList->SetComputeRootDescriptorTable(Ce_CullExclusiveSRVsParameterId, cullSrvHandle);

		// Pipeline + Constants
		commandList->SetPipelineState(drawInstCmdPso);
		commandList->SetComputeRoot32BitConstant(Ce_CullDrawCountParameterId, (UINT)lodDataCount, 0);

		// Sets commands with instancing
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize((uint32_t)lodDataCount, 64), 1, 1);

		// Blocks graphics, wait for count and command write and also instance buffer write
		D3D12_RESOURCE_BARRIER graphicsBarriers[5]{};
		CreateResourcesTransitionBarrier(graphicsBarriers[0], varBuffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourcesTransitionBarrier(graphicsBarriers[1], varBuffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourceUAVBarrier(graphicsBarriers[2], varBuffers.indirectDrawBuffer.buffer.Get());
		CreateResourceUAVBarrier(graphicsBarriers[3], varBuffers.indirectDrawCount.buffer.Get());
		CreateResourceUAVBarrier(graphicsBarriers[4], varBuffers.drawInstBuffer.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void ClearWindow(ID3D12GraphicsCommandList* cmdList, float swapchainWidth, float swapchainHeight, 
		ID3D12Resource* swapchainBackBuffer, ID3D12DescriptorHeap* rvtHeap, ID3D12DescriptorHeap* dsvHeap, 
		Dx12Renderer::DescriptorContext& descriptorContext, UINT swapchainIndex)
	{
		DefineViewportAndScissor(cmdList, swapchainWidth, swapchainHeight);

		// Render target barrier
		D3D12_RESOURCE_BARRIER renderTargetBarrier{};
		CreateResourcesTransitionBarrier(renderTargetBarrier, swapchainBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdList->ResourceBarrier(1, &renderTargetBarrier);

		// Render target bind
		auto rtvHandle = rvtHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += (descriptorContext.swapchainRtvOffset + swapchainIndex) * descriptorContext.rtvIncrementSize;
		auto dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		dsvHandle.ptr += (descriptorContext.depthTargetOffset + swapchainIndex) * descriptorContext.dsvIncrementSize;
		cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		cmdList->ClearRenderTargetView(rtvHandle, BlitzenEngine::Ce_DefaultWindowBackgroundColor, 0, nullptr);
		cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, Ce_ClearDepth, 0, 0, nullptr);
	}

	static void BeginRenderPass(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* swapchainBackBuffer, 
		ID3D12DescriptorHeap* rvtHeap, ID3D12DescriptorHeap* dsvHeap,Dx12Renderer::DescriptorContext& descriptorContext, 
		UINT swapchainIndex, uint8_t isFirstPass)
	{
		// Render target bind
		auto rtvHandle = rvtHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += (descriptorContext.swapchainRtvOffset + swapchainIndex) * descriptorContext.rtvIncrementSize;
		auto dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		dsvHandle.ptr += (descriptorContext.depthTargetOffset + swapchainIndex) * descriptorContext.dsvIncrementSize;
		cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthTargetDesc{};
		D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargetDesc{};
		depthTargetDesc.cpuDescriptor = dsvHandle;
		renderTargetDesc.cpuDescriptor = rtvHandle;
		if (isFirstPass)
		{
			depthTargetDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
			depthTargetDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = Ce_ClearDepth;
			depthTargetDesc.DepthBeginningAccess.Clear.ClearValue.Format = Ce_DepthTargetFormat;
			depthTargetDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

			renderTargetDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
			renderTargetDesc.BeginningAccess.Clear.ClearValue.Color[0] = BlitzenEngine::Ce_DefaultWindowBackgroundColor[0];
			renderTargetDesc.BeginningAccess.Clear.ClearValue.Color[1] = BlitzenEngine::Ce_DefaultWindowBackgroundColor[1];
			renderTargetDesc.BeginningAccess.Clear.ClearValue.Color[2] = BlitzenEngine::Ce_DefaultWindowBackgroundColor[2];
			renderTargetDesc.BeginningAccess.Clear.ClearValue.Color[3] = BlitzenEngine::Ce_DefaultWindowBackgroundColor[3];
			renderTargetDesc.BeginningAccess.Clear.ClearValue.Format = Ce_SwapchainFormat;
			renderTargetDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		}
		else
		{
			depthTargetDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
			depthTargetDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

			renderTargetDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
			renderTargetDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		}

		// Begin render pass
		cmdList->BeginRenderPass(1, &renderTargetDesc, &depthTargetDesc, D3D12_RENDER_PASS_FLAG_NONE);
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

			if (CE_DX12OCCLUSION)
			{
				for (uint32_t i = 0; i < ce_framesInFlight; ++i)
				{
					CreateDepthPyramidResource(m_device.Get(), varBuffers.depthPyramid, m_swapchainWidth, m_swapchainHeight);
				}

				RecreateDepthPyramidDescriptors(m_device.Get(), m_varBuffers, m_descriptorContext, m_srvHeap.Get(),
					m_depthBuffers, m_swapchainWidth, m_swapchainHeight);
			}

			context.pCamera->viewData.pyramidWidth = float(varBuffers.depthPyramid.width);
			context.pCamera->viewData.pyramidWidth = float(varBuffers.depthPyramid.height);
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
			// Frustum culling and lod selection, per object
			DrawOccFirstPass(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), m_descriptorContext.sharedSrvHandle[m_currentFrame],
				m_descriptorContext.cullSrvHandle[m_currentFrame], m_descriptorContext, m_currentFrame, m_drawCountResetRoot.Get(), m_drawCountResetPso.Get(),
				m_drawCullSignature.Get(), m_drawCullPso.Get(), varBuffers, context.pResources->renderObjectCount);

			// Render target barrier
			D3D12_RESOURCE_BARRIER renderTargetBarrier{};
			CreateResourcesTransitionBarrier(renderTargetBarrier, m_swapchainBackBuffers[swapchainIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			frameTools.mainGraphicsCommandList->ResourceBarrier(1, &renderTargetBarrier);
			uint8_t firstPass = 1;
			DefineViewportAndScissor(frameTools.mainGraphicsCommandList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);
			BeginRenderPass(frameTools.mainGraphicsCommandList.Get(), m_swapchainBackBuffers[swapchainIndex].Get(), m_rtvHeap.Get(), 
				m_dsvHeap.Get(), m_descriptorContext, swapchainIndex, firstPass);

			DrawIndirect(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), m_samplerHeap.Get(), m_opaqueRootSignature.Get(),
				m_opaqueGraphicsPso.Get(), m_opaqueCmdSingature.Get(), m_descriptorContext, m_constBuffers, varBuffers, m_currentFrame);
			frameTools.mainGraphicsCommandList->EndRenderPass();
			firstPass = 0;

			GenerateDepthPyramid(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), m_descriptorContext, m_currentFrame, swapchainIndex,
				m_depthPyramidSignature.Get(), m_depthPyramidPso.Get(), varBuffers, m_depthBuffers[swapchainIndex].Get(), m_samplerHeap.Get(), 
				m_swapchainWidth, m_swapchainHeight);

			DrawOccLatePass(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), m_descriptorContext.sharedSrvHandle[m_currentFrame], 
				m_descriptorContext.cullSrvHandle[m_currentFrame], m_descriptorContext, m_currentFrame, m_drawCountResetRoot.Get(), 
				m_drawCountResetPso.Get(), m_drawOccLateSignature.Get(), m_drawOccLatePso.Get(), varBuffers, context.pResources->renderObjectCount, 
				m_samplerHeap.Get());
			
			BeginRenderPass(frameTools.mainGraphicsCommandList.Get(), m_swapchainBackBuffers[swapchainIndex].Get(), m_rtvHeap.Get(),
				m_dsvHeap.Get(), m_descriptorContext, swapchainIndex, firstPass);
			
			DrawIndirect(frameTools.mainGraphicsCommandList.Get(), m_srvHeap.Get(), m_samplerHeap.Get(), m_opaqueRootSignature.Get(),
				m_opaqueGraphicsPso.Get(), m_opaqueCmdSingature.Get(), m_descriptorContext, m_constBuffers, varBuffers, m_currentFrame);
			frameTools.mainGraphicsCommandList->EndRenderPass();
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

#if defined(DX12_OCCLUSION_DRAW_CULL) && defined(BLIT_DEPTH_PYRAMID_TEST)
		{
			CopyDepthPyramidToSwapchain(frameTools.mainGraphicsCommandList.Get(), m_swapchainBackBuffers[swapchainIndex].Get(), varBuffers.depthPyramid.pyramid.Get(), 
				varBuffers.depthPyramid.width, varBuffers.depthPyramid.height, nullptr, m_commandQueue.Get(), m_swapchain.Get(), 
				context.pCamera->transformData.debugPyramidLevel, m_swapchainWidth, m_swapchainHeight);
		}
#else
		{
			Present(frameTools, m_swapchain.Get(), m_commandQueue.Get(), m_swapchainBackBuffers[swapchainIndex].Get());
		}
#endif

		m_currentFrame = (m_currentFrame + 1) % ce_framesInFlight;
    }

	void Dx12Renderer::DrawWhileWaiting(float deltaTime)
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