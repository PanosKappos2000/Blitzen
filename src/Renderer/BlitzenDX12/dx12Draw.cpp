#if defined(_WIN32)

#include "dx12Renderer.h"
#include "dx12RNDResources.h"

namespace BlitzenDX12
{
	static void UpdateBuffers(CmdContext& cmdContext, ReadWriteResources& rwResources, BlitzenEngine::Camera* pCamera, ID3D12CommandQueue* commandQueue)
	{
		if (pCamera->transformData.bFreezeFrustum)
		{
			// Only change the matrix that moves the camera if the freeze frustum debug functionality is active
			rwResources.m_viewBuffer.pData->projectionViewMatrix = pCamera->viewData.projectionViewMatrix;
		}
		else
		{
			*rwResources.m_viewBuffer.pData = pCamera->viewData;
		}

		cmdContext.m_copyCmdAlloc->Reset();
		cmdContext.m_copyCmdList->Reset(cmdContext.m_copyCmdAlloc.Get(), nullptr);
		
		cmdContext.m_copyCmdList->CopyBufferRegion(rwResources.m_transformBuffer.buffer.Get(), 0, rwResources.m_transformBuffer.staging.Get(), 0, rwResources.m_transformBuffer.dataCopySize);
		
		cmdContext.m_copyCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_copyCmdList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);
		
		PlaceFence(cmdContext.m_copyFence.m_value, commandQueue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);
	}

	static void RecreateSwapchain(BlitzenEngine::DrawContext& context, IDXGIFactory6* factory, ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t newWidth, uint32_t newHeight,
		DX12WRAPPER<IDXGISwapChain3>* pSwapchain, DX12WRAPPER<ID3D12Resource>* pSwapchainBuffers, DX12WRAPPER<ID3D12Resource>* pDepthTargets, 
		DescriptorContext& descriptorContext, CmdContext* pCmd)
	{
		// Waits for all frames
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& cmdContext{ pCmd[i] };
			PlaceFence(cmdContext.m_frameFence.m_value, queue, cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);

			pSwapchainBuffers[i]->Release();
			pDepthTargets[i]->Release();
		}

		pSwapchain->ReleaseAndGetAddressOf();
		HWND hwnd = context.m_pPlatform->m_hwnd;
		BLIT_ASSERT(CreateSwapchain(factory, queue, newWidth, newHeight, hwnd, pSwapchain));
		
		// Automatically releases when trying to create. Creates 
		for (UINT i = 0; i < ce_framesInFlight; i++)
		{
			SIZE_T offset{ descriptorContext.m_swapchainRtvOffset[i]};

			HRESULT getBackBufferResult = pSwapchain->Get()->GetBuffer(i, IID_PPV_ARGS(pSwapchainBuffers[i].GetAddressOf()));
			BLIT_ASSERT(SUCCEEDED(getBackBufferResult));

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = Ce_SwapchainFormat;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

			// Handle for heap start
			auto handle = descriptorContext.m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
			handle.ptr += offset * descriptorContext.m_rtvHeapIncrement;

			// Creates view
			device->CreateRenderTargetView(pSwapchainBuffers[i].Get(), &rtvDesc, handle);
		}
		
		for (UINT i = 0; i < ce_framesInFlight; i++)
		{
			SIZE_T offset{ descriptorContext.m_depthTargetDsvOffset[i]};

			D3D12_CLEAR_VALUE clear{};
			clear.Format = Ce_DepthTargetFormat;
			clear.DepthStencil.Depth = Ce_ClearDepth;

			HRESULT depthTargetRes{ CreateImageResource(device, pDepthTargets[i].GetAddressOf(), newWidth, newHeight, 1, Ce_DepthTargetFormat,
				D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear) };
			BLIT_ASSERT(SUCCEEDED(depthTargetRes));

			D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
			viewDesc.Format = Ce_DepthTargetFormat;
			viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			auto handle{ descriptorContext.m_dsvHeap->GetCPUDescriptorHandleForHeapStart() };
			handle.ptr += offset * descriptorContext.m_dsvHeapIncrement;

			device->CreateDepthStencilView(pDepthTargets[i].Get(), &viewDesc, handle);
		}
	}

	static void RecreateDepthPyramidDescriptors(ID3D12Device* device, ReadWriteResources* rwResourcesArray, DescriptorContext& context, UINT drawWidth, UINT drawHeight)
	{
		SIZE_T offset{ context.m_HI_Z_MapSRVOffset[0]};

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& rwResources = rwResourcesArray[i];

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = Ce_DepthPyramidFormat;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = rwResources.m_HI_Z.mipCount;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;

			// handle for heap start
			auto handle{ context.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
			handle.ptr += offset * context.m_viewHeapIncrement;

			// Creates view
			device->CreateShaderResourceView(rwResources.m_HI_Z.pyramid.Get(), &srvDesc, handle);

			// Increments offset
			offset++;
		}

		for (uint32_t f = 0; f < ce_framesInFlight; ++f)
		{
			auto& rwResources = rwResourcesArray[f];
			offset = context.m_HI_Z_MapMipsFirstUAVOffset[f];

			for (uint32_t hi_z_mip = 0; hi_z_mip < rwResources.m_HI_Z.mipCount; ++hi_z_mip)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
				uavDesc.Format = Ce_DepthPyramidFormat;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = hi_z_mip;
				uavDesc.Texture2D.PlaneSlice = 0;

				// Handle for heap start
				auto handle{ context.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
				handle.ptr += offset * context.m_viewHeapIncrement;

				// Creates view
				device->CreateUnorderedAccessView(rwResources.m_HI_Z.pyramid.Get(), nullptr, &uavDesc, handle);

				// Increments offset 
				offset++;
			}
		}
		
	}

	static void RecreateDepthTargetDescriptor(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* pDepthTargets, DescriptorContext& ctx)
	{
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			SIZE_T offset{ ctx.m_depthTargetSRVOffset[i] };

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = Ce_DepthTargetSRVFormat;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;

			// handle for heap start
			auto handle{ ctx.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
			handle.ptr += offset * ctx.m_viewHeapIncrement;

			// Creates view
			device->CreateShaderResourceView(pDepthTargets[i].Get(), &srvDesc, handle);

			offset++;
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

	static void DrawCountReset(ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* resetRoot, ID3D12PipelineState* resetPso, D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, 
		ReadWriteResources& rwResources)
	{
		// Reource barrier before count is reset and commands are rewritten
		D3D12_RESOURCE_BARRIER resetBarrier{};
		CreateResourcesTransitionBarrier(resetBarrier, rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(1, &resetBarrier);

		// Descriptors
		commandList->SetComputeRootSignature(resetRoot);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullExclusiveSRVsRootID, cullSrvHandle);

		// Pipeline + constants
		commandList->SetPipelineState(resetPso);

		commandList->Dispatch(1, 1, 1);
	}

	static void DrawCullPass(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources, 
		uint32_t objCount, uint32_t frame)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get() };
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

		// Culling barrier, waits for draw count reset and draw command read
		D3D12_RESOURCE_BARRIER cullingBarriers[2]{};
		// Count reset barrier 
		CreateResourceUAVBarrier(cullingBarriers[0], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		// Command read barrier
		CreateResourcesTransitionBarrier(cullingBarriers[1], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawCullRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);

		// Pipeline + constants
		commandList->SetPipelineState(pipelineContext.m_drawCullPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
		// Command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// Counter write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void DrawOccFirstPass(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources, 
		uint32_t objCount, uint32_t frame)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get() };
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

		// Culling barrier, waits for draw count reset and draw command read, and draw visibility write as well
		D3D12_RESOURCE_BARRIER cullingBarriers[3]{};
		// Count reset barrier 
		CreateResourceUAVBarrier(cullingBarriers[0], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		// Command read barrier
		CreateResourcesTransitionBarrier(cullingBarriers[1], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// Draw visibility barrier
		CreateResourceUAVBarrier(cullingBarriers[2], rwResources.m_drawVisBuffer.buffer.Get());
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawOccFirstRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccFirstSharedSRVsRootId, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccFirstExclusiveSRVsRootId, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccFirstDrawVisUAVRootId, descriptorContext.m_drawVisUANHandle[frame]);

		// Pipeline + constants
		commandList->SetPipelineState(pipelineContext.m_drawOccFirstPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
		// command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// count write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void GenerateHI_Z_MAP(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, uint32_t frame, UINT swapchainId, PipelineContext& pipelineContext, 
		ReadWriteResources& rwResources, ID3D12Resource* depthTarget, uint32_t swapchainWidth, uint32_t swapchainHeight)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* heaps[] = { descriptorContext.m_viewHeap.Get()};
		commandList->SetDescriptorHeaps(1, heaps);

		// Barrier for depth pyramid generation, waits for depth target write and HI Z map read
		D3D12_RESOURCE_BARRIER depthPyramidBarriers[2]{};
		// Depth target write
		CreateResourcesTransitionBarrier(depthPyramidBarriers[0], depthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		// HI Z map read
		CreateResourcesTransitionBarrier(depthPyramidBarriers[1], rwResources.m_HI_Z.pyramid.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(depthPyramidBarriers), depthPyramidBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_HI_Z_MapRoot.Get());
		commandList->SetPipelineState(pipelineContext.m_HI_Z_MapPso.Get());

		uint32_t mipLevel{ 0 };

		for (uint32_t i = 0; i < rwResources.m_HI_Z.mipCount; ++i)
		{
			// Mip size calculcations
			uint32_t levelWidth = BlitML::Max(1u, (rwResources.m_HI_Z.width) >> i);
			uint32_t levelHeight = BlitML::Max(1u, (rwResources.m_HI_Z.height) >> i);

			commandList->SetComputeRoot32BitConstant(Ce_HI_Z_MapMipLvlConstantRootID, mipLevel, 0);

			// Binds write texture (the depth pyramid has a copy for double buffering and each one has the correct offsets for the descriptor heap)
			commandList->SetComputeRootDescriptorTable(Ce_HI_Z_MapUAVRootID, rwResources.m_HI_Z.mips[i]);

			// Binds read texture (For first level, it's the depth target. For every other level, it's the depth pyramid itself)
			if (i == 0)
			{
				commandList->SetComputeRootDescriptorTable(Ce_HI_Z_MapSRVRootID, descriptorContext.m_depthTargetSRVHandle[swapchainId]);
			}
			else
			{
				commandList->SetComputeRootDescriptorTable(Ce_HI_Z_MapSRVRootID, descriptorContext.m_HI_Z_MapSRVHandle[frame]);
				mipLevel++;
			}

			// Generate level
			commandList->Dispatch(BlitML::GetComputeShaderGroupSize(levelWidth, 32), BlitML::GetComputeShaderGroupSize(levelHeight, 32), 1);

			// Barrier for the next loop, since it will use the current mip as the read descriptor
			D3D12_RESOURCE_BARRIER nextLoopBarrier{};
			CreateResourceUAVBarrier(nextLoopBarrier, rwResources.m_HI_Z.pyramid.Get());
			commandList->ResourceBarrier(1, &nextLoopBarrier);
		}

		// Culling waits for hi z write
		D3D12_RESOURCE_BARRIER cullingBarrier{};
		CreateResourcesTransitionBarrier(cullingBarrier, rwResources.m_HI_Z.pyramid.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &cullingBarrier);

		// Graphics wait for depth target read
		D3D12_RESOURCE_BARRIER graphicsBarrier{};
		CreateResourcesTransitionBarrier(graphicsBarrier, depthTarget, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandList->ResourceBarrier(1, &graphicsBarrier);
	}

	static void DrawOccLatePass(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources,
		uint32_t objCount, uint32_t frame)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get()};
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

		// Culling barrier, waits for draw count reset and draw command read, and draw visibility read as well
		// Finally, it needs to wait for the depth pyramid (occlusion culling)
		D3D12_RESOURCE_BARRIER cullingBarriers[3]{};
		// Count reset barrier 
		CreateResourceUAVBarrier(cullingBarriers[0], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		// Command read barrier
		CreateResourcesTransitionBarrier(cullingBarriers[1], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// Draw visibility barrier
		CreateResourceUAVBarrier(cullingBarriers[2], rwResources.m_drawVisBuffer.buffer.Get());
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawOccLateRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccLateSharedSRVsRootId, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccLateExclusiveSRVsRootId, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccLateDrawVisUAVRootId, descriptorContext.m_drawVisUANHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccLateHI_Z_MapRootId, descriptorContext.m_HI_Z_MapSRVHandle[frame]);

		// Pipeline + constants
		commandList->SetPipelineState(pipelineContext.m_drawOccLatePso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
		// command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// count write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void DrawOccTemporalPass(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources,
		uint32_t objCount, uint32_t frame)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get()};
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Draw count reset
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

		// Blocks culling shader, waits for indirect command read and count reset(the depth pyramid has a barrier in its generate function)
		D3D12_RESOURCE_BARRIER cullingBarriers[2]{};
		// Command ssbo
		CreateResourcesTransitionBarrier(cullingBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// Count reset
		CreateResourceUAVBarrier(cullingBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawOccLateRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccTemporalSharedSRVsRootId, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccTemporalExclusiveSRVsRootId, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccTemporalHI_Z_MapRootId, descriptorContext.m_HI_Z_MapSRVHandle[frame]);

		// Pipeline + root constants
		commandList->SetPipelineState(pipelineContext.m_drawOccTemporalPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
		// command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// count write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void ClusterCullDispatch(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources,
		BlitzenEngine::DrawContext& context, uint32_t frame)
	{

	}

	static void ClusterCull(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources,
		BlitzenEngine::DrawContext& context, uint32_t frame)
	{

	}

	static void DrawInstanceCullPass(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources,
		BlitzenEngine::DrawContext& context, uint32_t frame)
	{
		size_t lodDataCount{ context.m_meshes.m_LODs.GetSize()};
		uint32_t objCount{ context.m_renders.m_renderCount };

		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get()};
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

		// Blocks instance counter reset
		D3D12_RESOURCE_BARRIER instCounterResetBarrier{};
		CreateResourceUAVBarrier(instCounterResetBarrier, rwResources.m_instCounterBuffer.buffer.Get());
		commandList->ResourceBarrier(1, &instCounterResetBarrier);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawCullInstRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstSharedSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstAdditionalSRVsRootID, descriptorContext.m_drawCullInstUAVsHandle[frame]);

		// Pipeline + Constants
		commandList->SetPipelineState(pipelineContext.m_drawInstCountResetPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, (UINT)lodDataCount, 0);

		// Resets instance counters
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(uint32_t(lodDataCount), 64), 1, 1);

		// Culling barriers. Waits for draw and instance count reset, instance buffer and draw buffer read
		D3D12_RESOURCE_BARRIER cullingBarriers[4]{};
		// Draw commands barrier
		CreateResourcesTransitionBarrier(cullingBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// Draw count barrier
		CreateResourceUAVBarrier(cullingBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		// Instance Counter barrier
		CreateResourceUAVBarrier(cullingBarriers[2], rwResources.m_instCounterBuffer.buffer.Get());
		// Instance barrier
		CreateResourceUAVBarrier(cullingBarriers[3], rwResources.m_drawInstBuffer.buffer.Get());
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawCullInstRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstAdditionalSRVsRootID, descriptorContext.m_drawCullInstUAVsHandle[frame]);

		// Pipeline + constants
		commandList->SetPipelineState(pipelineContext.m_drawCullInstPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Blocks instancing until the instance counters are set
		D3D12_RESOURCE_BARRIER instancingBarrier{};
		CreateResourceUAVBarrier(instancingBarrier, rwResources.m_instCounterBuffer.buffer.Get());
		commandList->ResourceBarrier(1, &instancingBarrier);

		// Descriptors
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstAdditionalSRVsRootID, descriptorContext.m_drawCullInstUAVsHandle[frame]);

		// Pipeline + Constants
		commandList->SetPipelineState(pipelineContext.m_drawInstCmdPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, (UINT)lodDataCount, 0);

		// Sets commands with instancing
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize((uint32_t)lodDataCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[3]{};
		// command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// count write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// inst write
		CreateResourceUAVBarrier(graphicsBarriers[2], rwResources.m_drawInstBuffer.buffer.Get());
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void ClearWindow(ID3D12GraphicsCommandList* cmdList, float swapchainWidth, float swapchainHeight, 
		ID3D12Resource* swapchainBackBuffer, DescriptorContext& descriptorContext, UINT swapchainIndex)
	{
		DefineViewportAndScissor(cmdList, swapchainWidth, swapchainHeight);

		// Render target barrier
		D3D12_RESOURCE_BARRIER renderTargetBarrier{};
		CreateResourcesTransitionBarrier(renderTargetBarrier, swapchainBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdList->ResourceBarrier(1, &renderTargetBarrier);

		cmdList->OMSetRenderTargets(1, &descriptorContext.m_swapchainRtvHandle[swapchainIndex], FALSE, &descriptorContext.m_depthTargetDSVHandle[swapchainIndex]);

		cmdList->ClearRenderTargetView(descriptorContext.m_swapchainRtvHandle[swapchainIndex], BlitzenCore::Ce_DefaultWindowBackgroundColor, 0, nullptr);
		cmdList->ClearDepthStencilView(descriptorContext.m_depthTargetDSVHandle[swapchainIndex], D3D12_CLEAR_FLAG_DEPTH, Ce_ClearDepth, 0, 0, nullptr);
	}

	static void BeginRenderPass(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* swapchainBackBuffer, DescriptorContext& descriptorContext, 
		UINT swapchainIndex, uint8_t isFirstPass)
	{
		// Render target bind
		cmdList->OMSetRenderTargets(1, &descriptorContext.m_swapchainRtvHandle[swapchainIndex], FALSE, &descriptorContext.m_depthTargetDSVHandle[swapchainIndex]);

		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthTargetDesc{};
		D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargetDesc{};
		depthTargetDesc.cpuDescriptor = descriptorContext.m_depthTargetDSVHandle[swapchainIndex];
		renderTargetDesc.cpuDescriptor = descriptorContext.m_swapchainRtvHandle[swapchainIndex];
		if (isFirstPass)
		{
			depthTargetDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
			depthTargetDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = Ce_ClearDepth;
			depthTargetDesc.DepthBeginningAccess.Clear.ClearValue.Format = Ce_DepthTargetFormat;
			depthTargetDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

			renderTargetDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
			renderTargetDesc.BeginningAccess.Clear.ClearValue.Color[0] = BlitzenCore::Ce_DefaultWindowBackgroundColor[0];
			renderTargetDesc.BeginningAccess.Clear.ClearValue.Color[1] = BlitzenCore::Ce_DefaultWindowBackgroundColor[1];
			renderTargetDesc.BeginningAccess.Clear.ClearValue.Color[2] = BlitzenCore::Ce_DefaultWindowBackgroundColor[2];
			renderTargetDesc.BeginningAccess.Clear.ClearValue.Color[3] = BlitzenCore::Ce_DefaultWindowBackgroundColor[3];
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

	static void DrawIndirect(ID3D12GraphicsCommandList* commandList, PipelineContext& pipelineContext, DescriptorContext& descriptorContext, 
		ReadOnlyResources& roResources, ReadWriteResources& rwResources, UINT frame)
	{
		ID3D12DescriptorHeap* graphicsHeaps[] = { descriptorContext.m_viewHeap.Get(), descriptorContext.m_samplerHeap.Get()};
		commandList->SetDescriptorHeaps(2, graphicsHeaps);
		commandList->SetGraphicsRootSignature(pipelineContext.m_opaqueDrawRoot.Get());

		// DESCRIPTORS
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawExclusiveSRVsRootID, descriptorContext.m_opaqueDrawViewsExclusiveHandle[frame]);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawTexSMPRootID, descriptorContext.m_texSmpHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawMatSRVRootID, descriptorContext.m_materialSRVHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawTexSRVRootID, descriptorContext.m_texDescriptorsSRVHandle);

		commandList->SetPipelineState(pipelineContext.m_opaqueDrawPso.Get());

		commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetIndexBuffer(&roResources.m_idxBuffer.m_view);

		// DRAW
		commandList->ExecuteIndirect(pipelineContext.m_opaqueDrawCmdSign.Get(), Ce_IndirectDrawCmdBufferSize, rwResources.m_drawCmdBuffer.buffer.Get(), 0, rwResources.m_drawCmdCounterBuffer.buffer.Get(), 0);
	}

	static void DrawOpaqueInst(ID3D12GraphicsCommandList* commandList, PipelineContext& pipelineContext, DescriptorContext& descriptorContext,
		ReadOnlyResources& roResources, ReadWriteResources& rwResources, UINT frame)
	{
		ID3D12DescriptorHeap* graphicsHeaps[] = { descriptorContext.m_viewHeap.Get(), descriptorContext.m_samplerHeap.Get() };
		commandList->SetDescriptorHeaps(2, graphicsHeaps);
		commandList->SetGraphicsRootSignature(pipelineContext.m_opaqueDrawInstRoot.Get());

		// DESCRIPTORS
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstExclusiveSRVsRootID, descriptorContext.m_opaqueDrawViewsExclusiveHandle[frame]);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstTexSMPRootID, descriptorContext.m_texSmpHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstMatSRVRootID, descriptorContext.m_materialSRVHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstTexSRVRootID, descriptorContext.m_texDescriptorsSRVHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstInstSRVRootID, descriptorContext.m_drawCullInstUAVsHandle[frame]);

		commandList->SetPipelineState(pipelineContext.m_opaqueDrawInstPso.Get());

		commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetIndexBuffer(&roResources.m_idxBuffer.m_view);

		// DRAW
		commandList->ExecuteIndirect(pipelineContext.m_opaqueDrawInstCmdSign.Get(), Ce_IndirectDrawCmdBufferSize, rwResources.m_drawCmdBuffer.buffer.Get(), 0, rwResources.m_drawCmdCounterBuffer.buffer.Get(), 0);
	}

	static void Present(ID3D12GraphicsCommandList* commandList, IDXGISwapChain3* swapchain, ID3D12CommandQueue* commandQueue, ID3D12Resource* swapchainBackBuffer)
	{
		D3D12_RESOURCE_BARRIER presentBarrier{};
		CreateResourcesTransitionBarrier(presentBarrier, swapchainBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &presentBarrier);

		commandList->Close();
		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(1, commandLists);

		swapchain->Present(1, 0);
	}

	void Dx12Renderer::Update(const BlitzenEngine::DrawContext& context)
	{

	}

	void Dx12Renderer::UpdateObjectTransform(uint32_t transformId, BlitzenEngine::MeshTransform* pTransform)
	{
		auto pData = m_rwResources[m_currentFrame].m_transformBuffer.pData;
		BlitzenCore::BlitMemCopy(reinterpret_cast<BlitzenEngine::MeshTransform*>(pData) + transformId, pTransform, sizeof(BlitzenEngine::MeshTransform));
	}

	void Dx12Renderer::DrawFrame(BlitzenEngine::DrawContext& context)
	{
		auto& cmdContext = m_cmdContext[m_currentFrame];
		auto& rwResources = m_rwResources[m_currentFrame];

		// LAST FRAME FENCE
		PlaceFence(cmdContext.m_frameFence.m_value, m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);

		// Render and Depth target resource recreation in case of window resize
		if (context.m_camera.transformData.bWindowResize)
		{
			m_swapchainWidth = (UINT)context.m_camera.transformData.windowWidth;
			m_swapchainHeight = (UINT)context.m_camera.transformData.windowHeight;

			RecreateSwapchain(context, m_factory.Get(), m_device.Get(), m_commandQueue.Get(), m_swapchainWidth, m_swapchainHeight, &m_swapchain, 
				m_swapchainBackBuffers, m_depthBuffers, m_descriptorContext, m_cmdContext);

			if (CE_DX12OCCLUSION)
			{
				for (uint32_t i = 0; i < ce_framesInFlight; ++i)
				{
					CreateDepthPyramidResource(m_device.Get(), m_rwResources[i].m_HI_Z, m_swapchainWidth, m_swapchainHeight);
				}

				RecreateDepthPyramidDescriptors(m_device.Get(), m_rwResources, m_descriptorContext, m_swapchainWidth, m_swapchainHeight);

				RecreateDepthTargetDescriptor(m_device.Get(), m_depthBuffers, m_descriptorContext);
			}

			context.m_camera.viewData.pyramidWidth = float(rwResources.m_HI_Z.width);
			context.m_camera.viewData.pyramidHeight = float(rwResources.m_HI_Z.height);
		}

		// DYNAMIC BUFFERS UPDATE
		UpdateBuffers(cmdContext, rwResources, &context.m_camera, m_transferCommandQueue.Get());

		// swapchain index
		UINT swapchainIndex = m_swapchain->GetCurrentBackBufferIndex();

		// RECORDING
		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), nullptr);

		// Restores transform buffer for graphics
		D3D12_RESOURCE_BARRIER transformAfterCopyBarrier{};
		CreateResourcesTransitionBarrier(transformAfterCopyBarrier, rwResources.m_transformBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &transformAfterCopyBarrier);

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			ClusterCullDispatch(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context, m_currentFrame);

			ClusterCull(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context, m_currentFrame);

			// Render target barrier
			D3D12_RESOURCE_BARRIER renderTargetBarrier{};
			CreateResourcesTransitionBarrier(renderTargetBarrier, m_swapchainBackBuffers[swapchainIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &renderTargetBarrier);

			// Viewport and scissor
			DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

			// One pass
			const uint8_t singlePass = 1;
			BeginRenderPass(cmdContext.m_graphicsCmdList.Get(), m_swapchainBackBuffers[swapchainIndex].Get(), m_descriptorContext, swapchainIndex, singlePass);

			// Ends pass
			cmdContext.m_graphicsCmdList->EndRenderPass();

			GenerateHI_Z_MAP(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_currentFrame, swapchainIndex, m_pipelineContext, rwResources,
				m_depthBuffers[swapchainIndex].Get(), m_swapchainWidth, m_swapchainHeight);
		}

		else if constexpr (CE_DX12TEMPORAL_OCCLUSION)
		{
			// Culling with Occlusion using previous frame depth pyramid
			DrawOccTemporalPass(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context.m_renders.m_renderCount, m_currentFrame);

			// Render target barrier
			D3D12_RESOURCE_BARRIER renderTargetBarrier{};
			CreateResourcesTransitionBarrier(renderTargetBarrier, m_swapchainBackBuffers[swapchainIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &renderTargetBarrier);

			// Viewport and scissor
			DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

			// One pass
			const uint8_t singlePass = 1;
			BeginRenderPass(cmdContext.m_graphicsCmdList.Get(), m_swapchainBackBuffers[swapchainIndex].Get(), m_descriptorContext, swapchainIndex, singlePass);

			DrawIndirect(cmdContext.m_graphicsCmdList.Get(), m_pipelineContext, m_descriptorContext, m_roResources, rwResources, m_currentFrame);

			// Ends pass
			cmdContext.m_graphicsCmdList->EndRenderPass();

			GenerateHI_Z_MAP(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_currentFrame, swapchainIndex, m_pipelineContext, rwResources,
				m_depthBuffers[swapchainIndex].Get(), m_swapchainWidth, m_swapchainHeight);
		}

		/*
			-FRUSTUM CULLING AND LOD SELECTION AT RENDER OBJECT LEVEL BEFORE DRAW INDIRECT
			-FIRST PASS CULLS PREVIOSLY VISIBLE OBJECTS TO CREATE OCCLUDERS
			-OCCLUDER COPIED TO HI-Z MAP
			-HI-Z MAP USED FOR SECOND CULLING PASS WHICH PERFORMS OCCLUSION CULLING
		*/
		else if constexpr (CE_DX12OCCLUSION)
		{
			// 1. FRUSTUM CULLING AND LOD SELECTION. COMMANDS CREATED FOR VISIBLE OBJECTS BASED ON THE SELECTED LOD
			// ONLY OBJECTS THAT WERE TAGGED AS VISIBLE LAST FRAME ARE CHECKED
			// Shaders used: drawCountReset.cs.hlsl + drawOccFirst.cs.hlsl
			DrawOccFirstPass(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context.m_renders.m_renderCount, m_currentFrame);

			// Render target barrier
			D3D12_RESOURCE_BARRIER renderTargetBarrier{};
			CreateResourcesTransitionBarrier(renderTargetBarrier, m_swapchainBackBuffers[swapchainIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &renderTargetBarrier);

			// First pass
			uint8_t firstPass = 1;

			// Viewport and scissor
			DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

			// 2. BEGINS RENDERING. For the first pass the render target and depth target are cleared and stored for the second pass
			BeginRenderPass(cmdContext.m_graphicsCmdList.Get(), m_swapchainBackBuffers[swapchainIndex].Get(), m_descriptorContext, swapchainIndex, firstPass);

			// 3. TAKES THE COMMNANDS FROM THE DRAW CULL SHADER AND DRAWS THE SCENE. THIS ALSO CREATES THE OCCLUDERS
			// Shaders used: opaqueDraw.vs.hlsl + opaqueDraw.ps.hlsl
			DrawIndirect(cmdContext.m_graphicsCmdList.Get(), m_pipelineContext, m_descriptorContext, m_roResources, rwResources, m_currentFrame);

			// Ends pass
			cmdContext.m_graphicsCmdList->EndRenderPass();
			firstPass = 0;

			// 4. DEPTH TARGET IS COPIED TO A HI-Z MAP
			GenerateHI_Z_MAP(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_currentFrame, swapchainIndex, m_pipelineContext, rwResources,
				m_depthBuffers[swapchainIndex].Get(), m_swapchainWidth, m_swapchainHeight);

			// 5. SECOND PASS DOES THE SAME AS THE OTHER PASS + OCCLUSION CULLING, OBJECTS THAT ARE NOW VISIBLE BUT WERE TAGGED AS NOT VISIBLE BEFORE GET DRAW COMMANDS
			// 6. ALL OBJECTS THAT ARE VISIBLE GET TAGGED AS VISIBLE FOR THE NEXT FRAME
			// Shaders used: drawCountReset.cs.hlsl + drawOccLate.cs.hlsl
			DrawOccLatePass(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context.m_renders.m_renderCount, m_currentFrame);
			
			// 7. BEGINS SECOND RENDER PASS. RENDER TARGET AND DEPTH TARGET ARE NOT CLEARED, BUT LOADED
			BeginRenderPass(cmdContext.m_graphicsCmdList.Get(), m_swapchainBackBuffers[swapchainIndex].Get(), m_descriptorContext, swapchainIndex, firstPass);
			
			// 8. DRAWS FOR THE SECOND PASS. SAME THING AS STEP 3
			DrawIndirect(cmdContext.m_graphicsCmdList.Get(), m_pipelineContext, m_descriptorContext, m_roResources, rwResources, m_currentFrame);

			cmdContext.m_graphicsCmdList->EndRenderPass();
		}

		/* 
			-FRUSTUM CULLING AND LOD SELECTION AT RENDER OBJECT LEVEL BEFORE DRAW INDIRECT
			-INSTANCE COUNTER USED TO CREATE FEWER DRAW COMMANDS
		*/
		else if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			// 1. FRUSTUM CULLING AND LOD SELECTION. INSTANCE COUNTER INCREMENTED EACH TIME AN LOD IS VISIBLE
			// 2. PASS OVER INSTANCE COUNTER, TO CREATE DRAW COMMANDS WITH INSTANCE COUNT
			// Shaders used: drawCountReset.cs.hlsl + drawInstCountReset.cs.hlsl + drawInstCull.cs.hlsl + drawInstCmd.cs.hlsl
			DrawInstanceCullPass(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context, m_currentFrame);

			// 3. BEGINS RENDERING
			ClearWindow(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight, m_swapchainBackBuffers[swapchainIndex].Get(), m_descriptorContext, swapchainIndex);

			// 4. TAKES THE COMMNADS FROM THE DRAW CULL SHADER AND DRAWS THE SCENE
			// Shaders used: opaqueDrawInst.vs.hlsl + opaqueDraw.ps.hlsl
			DrawOpaqueInst(cmdContext.m_graphicsCmdList.Get(), m_pipelineContext, m_descriptorContext, m_roResources, rwResources, m_currentFrame);
		}

		/* 
			FRUSTUM CULLING AND LOD SELECTION AT RENDER OBJECT LEVEL BEFORE DRAW INDIRECT 
		*/
		else
		{
			// 1. FRUSTUM CULLING AND LOD SELCTION. COMMANDS CREATED FOR VISIBLE OBJECTS BASED ON THE SELECTED LOD
			// Shaders used: drawCountReset.cs.hlsl + drawCull.cs.hlsl
			DrawCullPass(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context.m_renders.m_renderCount, m_currentFrame);

			// 2. BEGINS RENDERING
			ClearWindow(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight, m_swapchainBackBuffers[swapchainIndex].Get(), m_descriptorContext, swapchainIndex);

			// 3. TAKES THE COMMNANDS FROM THE DRAW CULL SHADER AND DRAWS THE SCENE
			// Shaders used: opaqueDraw.vs.hlsl + opaqueDraw.ps.hlsl
			DrawIndirect(cmdContext.m_graphicsCmdList.Get(), m_pipelineContext, m_descriptorContext, m_roResources, rwResources, m_currentFrame);
		}

		// Prepares transform buffer for next frame data copy
		D3D12_RESOURCE_BARRIER transformCopyBarrier{};
		CreateResourcesTransitionBarrier(transformCopyBarrier, rwResources.m_transformBuffer.buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &transformCopyBarrier);

		// Depth pyramid debugging
		#if defined(DX12_OCCLUSION_DRAW_CULL) && defined(BLIT_DEPTH_PYRAMID_TEST)

		CopyDepthPyramidToSwapchain(frameTools.mainGraphicsCommandList.Get(), m_swapchainBackBuffers[swapchainIndex].Get(), varBuffers.depthPyramid.pyramid.Get(), 
			varBuffers.depthPyramid.width, varBuffers.depthPyramid.height, nullptr, m_commandQueue.Get(), m_swapchain.Get(), context.pCamera->transformData.debugPyramidLevel, 
			m_swapchainWidth, m_swapchainHeight);

		#else
			
		Present(cmdContext.m_graphicsCmdList.Get(), m_swapchain.Get(), m_commandQueue.Get(), m_swapchainBackBuffers[swapchainIndex].Get());
		
		#endif

		m_currentFrame = (m_currentFrame + 1) % ce_framesInFlight;
    }

	void Dx12Renderer::DrawWhileWaiting(float deltaTime)
	{
		auto& cmdContext = m_cmdContext[m_currentFrame];

		UINT swapchainIndex = m_swapchain->GetCurrentBackBufferIndex();

		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), m_pipelineContext.m_trianglePso.Get());

		cmdContext.m_graphicsCmdList->SetGraphicsRootSignature(m_pipelineContext.m_triangleRoot.Get());
		DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

		D3D12_RESOURCE_BARRIER attachmentBarrier{};
		CreateResourcesTransitionBarrier(attachmentBarrier, m_swapchainBackBuffers[swapchainIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &attachmentBarrier);

		cmdContext.m_graphicsCmdList->OMSetRenderTargets(1, &m_descriptorContext.m_swapchainRtvHandle[m_currentFrame], FALSE, nullptr);

		cmdContext.m_graphicsCmdList->ClearRenderTargetView(m_descriptorContext.m_swapchainRtvHandle[m_currentFrame], BlitzenCore::Ce_DefaultWindowBackgroundColor, 0, nullptr);

		//BlitML::vec3 triangleColor{ 0, 0.8f, 0.4f };
		//frameTools.mainGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 3, &triangleColor, 0);
		cmdContext.m_graphicsCmdList->SetPipelineState(m_pipelineContext.m_trianglePso.Get());
		cmdContext.m_graphicsCmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdContext.m_graphicsCmdList->DrawInstanced(3, 1, 0, 0);

		Present(cmdContext.m_graphicsCmdList.Get(), m_swapchain.Get(), m_commandQueue.Get(), m_swapchainBackBuffers[swapchainIndex].Get());

		PlaceFence(cmdContext.m_frameFence.m_value, m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);

		m_currentFrame = (m_currentFrame + 1) % ce_framesInFlight;
	}
}

#endif