#if defined(_WIN32)
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Core/DbLog/blitAssert.h"

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
		
		cmdContext.m_copyCmdList->CopyBufferRegion(rwResources.m_transformBuffer.m_ssbo.buffer.Get(), 0, rwResources.m_transformBuffer.m_dynamicDataStaging.m_buffer.Get(), 0, 
			rwResources.m_transformBuffer.m_dynamicDataStaging.m_dataSize);
		
		cmdContext.m_copyCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_copyCmdList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);
		
		PlaceFence(cmdContext.m_copyFence.m_value, commandQueue, cmdContext.m_copyFence.m_dx12Handle.Get(), cmdContext.m_copyFence.m_event);
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
		ID3D12DescriptorHeap* srvHeaps[]{ descriptorContext.m_viewHeap.Get() };
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Unordered access counter
		D3D12_RESOURCE_BARRIER dispatchResetBarrier{};
		CreateResourcesTransitionBarrier(dispatchResetBarrier, rwResources.m_clusterDispatchBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(1, &dispatchResetBarrier);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_clusterCullRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);

		// Pipeline
		commandList->SetPipelineState(pipelineContext.m_clusterCullCmdResetPso.Get());

		// DISPATCH COUNTER RESET
		commandList->Dispatch(1, 1, 1);

		// Blocks dispatch, waits for cluster dispatch counter reset and dispatch cmd read
		D3D12_RESOURCE_BARRIER clusterDispatchBarriers[2]{};
		// dispatch counter reset
		CreateResourceUAVBarrier(clusterDispatchBarriers[0], rwResources.m_clusterDispatchBuffer.buffer.Get());
		// dispatch group data read
		CreateResourceUAVBarrier(clusterDispatchBarriers[1], rwResources.m_clusterGroupDataBuffer.buffer.Get());
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(clusterDispatchBarriers), clusterDispatchBarriers);

		// Descriptors
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullHI_Z_MapSrvRootID, descriptorContext.m_HI_Z_MapSRVHandle[frame]);

		// Pipelines and constants
		commandList->SetPipelineState(pipelineContext.m_clusterCullDispatchPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_ClusterCullDrawCountRootID, context.m_pResidents->m_renders.m_opaqueStaticCount, 0);
		
		// CULL DRAWS
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(context.m_pResidents->m_renders.m_opaqueStaticCount, 64), 1, 1);
	}

	static void ClusterCull(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources,
		BlitzenEngine::DrawContext& context, uint32_t frame)
	{
		ID3D12DescriptorHeap* srvHeaps[]{ descriptorContext.m_viewHeap.Get() };
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Blocks culling, waits for cluster dispatch count write and cluster dispatch cmd write, draw cmd read and draw cmd count reset
		D3D12_RESOURCE_BARRIER clusterCullBarriers[3]{};
		// cluster dispatch count
		CreateResourceUAVBarrier(clusterCullBarriers[0], rwResources.m_clusterGroupDataBuffer.buffer.Get());
		// cluster dispatch cmd
		CreateResourcesTransitionBarrier(clusterCullBarriers[1], rwResources.m_clusterDispatchBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// draw cmd count reset
		CreateResourceUAVBarrier(clusterCullBarriers[2], rwResources.m_clusterVisibilityBuffer.buffer.Get());
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(clusterCullBarriers), clusterCullBarriers);

		// descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_clusterCullRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullHI_Z_MapSrvRootID, descriptorContext.m_HI_Z_MapSRVHandle[frame]);

		commandList->SetPipelineState(pipelineContext.m_clusterCullPso.Get());

		// CULL CLUSTERS
		commandList->ExecuteIndirect(pipelineContext.m_clusterCullCmdSign.Get(), 1, rwResources.m_clusterDispatchBuffer.buffer.Get(), 0, nullptr, 0);
	}

	static void ClusterBatch(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources,
		BlitzenEngine::DrawContext& context, uint32_t frame)
	{
		ID3D12DescriptorHeap* srvHeaps[]{ descriptorContext.m_viewHeap.Get() };
		commandList->SetDescriptorHeaps(1, srvHeaps);
		
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);
		
		D3D12_RESOURCE_BARRIER clusterBatchCmdBarrier{};
		CreateResourcesTransitionBarrier(clusterBatchCmdBarrier, rwResources.m_clusterDispatchBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(1, &clusterBatchCmdBarrier);
		
		commandList->SetComputeRootSignature(pipelineContext.m_clusterCullRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);
		
		commandList->SetPipelineState(pipelineContext.m_clusterCullBatchCmdPso.Get());
		commandList->Dispatch(1, 1, 1);
		
		D3D12_RESOURCE_BARRIER clusterBatchBarriers[5]{};
		CreateResourcesTransitionBarrier(clusterBatchBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CreateResourceUAVBarrier(clusterBatchBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		CreateResourcesTransitionBarrier(clusterBatchBarriers[2], rwResources.m_clusterDispatchBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		CreateResourceUAVBarrier(clusterBatchBarriers[3], rwResources.m_clusterVisibilityBuffer.buffer.Get());
		CreateResourceUAVBarrier(clusterBatchBarriers[4], rwResources.m_clusterGroupDataBuffer.buffer.Get());
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(clusterBatchBarriers), clusterBatchBarriers);
		
		// descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_clusterCullRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);
		
		commandList->SetPipelineState(pipelineContext.m_clusterCullBatchPso.Get());
		
		// BATCH CLUSTERS
		commandList->ExecuteIndirect(pipelineContext.m_clusterCullCmdSign.Get(), 1, rwResources.m_clusterDispatchBuffer.buffer.Get(), 0, nullptr, 0);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
		// command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// count write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void DrawInstanceCullPass(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources,
		BlitzenEngine::DrawContext& context, uint32_t frame)
	{
		BLIT_ASSERT_MESSAGE(true, "Instancing is Under Reconstruction");
		uint32_t lodDataCount{ context.m_meshes.m_meshPrimitives.m_LODCount };
		uint32_t objCount{ context.m_pResidents->m_renders.m_opaqueStaticCount };

		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get()};
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

		// Blocks instance counter reset
		D3D12_RESOURCE_BARRIER instCounterResetBarrier{};
		//CreateResourceUAVBarrier(instCounterResetBarrier, rwResources.m_instCmdBuffer.buffer.Get());
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
		// Draw count barrier
		CreateResourceUAVBarrier(cullingBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		// Instance Counter barrier
		//CreateResourceUAVBarrier(cullingBarriers[2], rwResources.m_instCmdBuffer.buffer.Get());
		// Instance barrier
		//CreateResourceUAVBarrier(cullingBarriers[3], rwResources.m_drawInstBuffer.buffer.Get());
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

		// Sets commands with instancing
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize((uint32_t)lodDataCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[3]{};
		// command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// count write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// inst write
		//CreateResourcesTransitionBarrier(instanceBuffer)
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
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
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawMatSRVRootID, descriptorContext.m_opaqueDrawPSExclusiveViewsHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawTexSRVRootID, descriptorContext.m_texDescriptorsSRVHandle);

		commandList->SetPipelineState(pipelineContext.m_opaqueDrawPso.Get());

		commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetIndexBuffer(&roResources.m_idxBuffer.m_view);

		// DRAW
		commandList->ExecuteIndirect(pipelineContext.m_opaqueDrawCmdSign.Get(), Ce_IndirectDrawCmdBufferSize, rwResources.m_drawCmdBuffer.buffer.Get(), 0, 
			rwResources.m_drawCmdCounterBuffer.buffer.Get(), 0);
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
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstMatSRVRootID, descriptorContext.m_opaqueDrawPSExclusiveViewsHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstTexSRVRootID, descriptorContext.m_texDescriptorsSRVHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstInstSRVRootID, descriptorContext.m_drawCullInstUAVsHandle[frame]);

		commandList->SetPipelineState(pipelineContext.m_opaqueDrawInstPso.Get());

		commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetIndexBuffer(&roResources.m_idxBuffer.m_view);

		// DRAW
		commandList->ExecuteIndirect(pipelineContext.m_opaqueDrawInstCmdSign.Get(), Ce_IndirectDrawCmdBufferSize, rwResources.m_drawCmdBuffer.buffer.Get(), 0, 
			rwResources.m_drawCmdCounterBuffer.buffer.Get(), 0);
	}

	static void DrawIntancedDrive()
	{

	}

	static void ClusterDrive()
	{

	}

	static void TemporalOcclusionDrive()
	{

	}

	static void DoublePassOcclusionDrive()
	{

	}

	static void FrustumOnlyDrive()
	{

	}

	void Dx12Renderer::DrawFrame(BlitzenEngine::DrawContext& context)
	{
		auto& cmdContext = m_cmdContext[m_currentFrame];
		auto& rwResources = m_rwResources[m_currentFrame];

		// LAST FRAME FENCE
		PlaceFence(cmdContext.m_frameFence.m_value, m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);

		// DYNAMIC BUFFERS UPDATE
		UpdateBuffers(cmdContext, rwResources, &context.m_camera, m_transferCommandQueue.Get());

		// swapchain index
		m_swapchainIDX = m_swapchain->GetCurrentBackBufferIndex();

		// RECORDING
		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), nullptr);

		// Restores transform buffer for graphics
		D3D12_RESOURCE_BARRIER transformAfterCopyBarrier{};
		CreateResourcesTransitionBarrier(transformAfterCopyBarrier, rwResources.m_transformBuffer.m_ssbo.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &transformAfterCopyBarrier);

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			GenerateHI_Z_MAP(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_currentFrame, m_swapchainIDX, m_pipelineContext, rwResources,
				m_depthBuffers[m_swapchainIDX].Get(), m_swapchainWidth, m_swapchainHeight);

			ClusterCullDispatch(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context, m_currentFrame);

			ClusterCull(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context, m_currentFrame);

			ClusterBatch(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context, m_currentFrame);

			// Render target barrier
			D3D12_RESOURCE_BARRIER renderTargetBarrier{};
			CreateResourcesTransitionBarrier(renderTargetBarrier, m_swapchainBackBuffers[m_swapchainIDX].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &renderTargetBarrier);

			// Viewport and scissor
			DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

			BeginRenderPassClear(cmdContext.m_graphicsCmdList.Get(), m_swapchainBackBuffers[m_swapchainIDX].Get(), m_descriptorContext, m_pipelineContext, m_swapchainIDX);

			DrawIndirect(cmdContext.m_graphicsCmdList.Get(), m_pipelineContext, m_descriptorContext, m_roResources, rwResources, m_currentFrame);

			// Ends pass
			cmdContext.m_graphicsCmdList->EndRenderPass();
		}

		else if constexpr (BlitzenCore::Ce_DrawTemporalOcclusion)
		{
			// Culling with Occlusion using previous frame depth pyramid
			DrawOccTemporalPass(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context.m_pResidents->m_renders.m_opaqueStaticCount, m_currentFrame);

			// Render target barrier
			D3D12_RESOURCE_BARRIER renderTargetBarrier{};
			CreateResourcesTransitionBarrier(renderTargetBarrier, m_swapchainBackBuffers[m_swapchainIDX].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &renderTargetBarrier);

			// Viewport and scissor
			DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

			BeginRenderPassClear(cmdContext.m_graphicsCmdList.Get(), m_swapchainBackBuffers[m_swapchainIDX].Get(), m_descriptorContext, m_pipelineContext, m_swapchainIDX);

			DrawIndirect(cmdContext.m_graphicsCmdList.Get(), m_pipelineContext, m_descriptorContext, m_roResources, rwResources, m_currentFrame);

			// Ends pass
			cmdContext.m_graphicsCmdList->EndRenderPass();

			GenerateHI_Z_MAP(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_currentFrame, m_swapchainIDX, m_pipelineContext, rwResources,
				m_depthBuffers[m_swapchainIDX].Get(), m_swapchainWidth, m_swapchainHeight);
		}

		/*
			-FRUSTUM CULLING AND LOD SELECTION AT RENDER OBJECT LEVEL BEFORE DRAW INDIRECT
			-FIRST PASS CULLS PREVIOSLY VISIBLE OBJECTS TO CREATE OCCLUDERS
			-OCCLUDER COPIED TO HI-Z MAP
			-HI-Z MAP USED FOR SECOND CULLING PASS WHICH PERFORMS OCCLUSION CULLING
		*/
		else if constexpr (BlitzenCore::Ce_OcclusionCulling)
		{
			// 1. FRUSTUM CULLING AND LOD SELECTION. COMMANDS CREATED FOR VISIBLE OBJECTS BASED ON THE SELECTED LOD
			// ONLY OBJECTS THAT WERE TAGGED AS VISIBLE LAST FRAME ARE CHECKED
			// Shaders used: drawCountReset.cs.hlsl + drawOccFirst.cs.hlsl
			DrawOccFirstPass(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context.m_pResidents->m_renders.m_opaqueStaticCount, m_currentFrame);

			// Render target barrier
			D3D12_RESOURCE_BARRIER renderTargetBarrier{};
			CreateResourcesTransitionBarrier(renderTargetBarrier, m_swapchainBackBuffers[m_swapchainIDX].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &renderTargetBarrier);

			// Viewport and scissor
			DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

			// 2. BEGINS RENDERING. For the first pass the render target and depth target are cleared and stored for the second pass
			BeginRenderPassClear(cmdContext.m_graphicsCmdList.Get(), m_swapchainBackBuffers[m_swapchainIDX].Get(), m_descriptorContext, m_pipelineContext, m_swapchainIDX);

			// 3. TAKES THE COMMNANDS FROM THE DRAW CULL SHADER AND DRAWS THE SCENE. THIS ALSO CREATES THE OCCLUDERS
			// Shaders used: opaqueDraw.vs.hlsl + opaqueDraw.ps.hlsl
			DrawIndirect(cmdContext.m_graphicsCmdList.Get(), m_pipelineContext, m_descriptorContext, m_roResources, rwResources, m_currentFrame);

			// Ends pass
			cmdContext.m_graphicsCmdList->EndRenderPass();

			// 4. DEPTH TARGET IS COPIED TO A HI-Z MAP
			GenerateHI_Z_MAP(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_currentFrame, m_swapchainIDX, m_pipelineContext, rwResources,
				m_depthBuffers[m_swapchainIDX].Get(), m_swapchainWidth, m_swapchainHeight);

			// 5. SECOND PASS DOES THE SAME AS THE OTHER PASS + OCCLUSION CULLING, OBJECTS THAT ARE NOW VISIBLE BUT WERE TAGGED AS NOT VISIBLE BEFORE GET DRAW COMMANDS
			// 6. ALL OBJECTS THAT ARE VISIBLE GET TAGGED AS VISIBLE FOR THE NEXT FRAME
			// Shaders used: drawCountReset.cs.hlsl + drawOccLate.cs.hlsl
			DrawOccLatePass(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context.m_pResidents->m_renders.m_opaqueStaticCount, m_currentFrame);
			
			// 7. BEGINS SECOND RENDER PASS. RENDER TARGET AND DEPTH TARGET ARE NOT CLEARED, BUT LOADED
			BeginRenderPassPreserve(cmdContext.m_graphicsCmdList.Get(), m_swapchainBackBuffers[m_swapchainIDX].Get(), m_descriptorContext, m_pipelineContext, m_swapchainIDX);
			
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
			ClearWindow(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight, m_swapchainBackBuffers[m_swapchainIDX].Get(), m_descriptorContext, m_swapchainIDX);

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
			DrawCullPass(cmdContext.m_graphicsCmdList.Get(), m_descriptorContext, m_pipelineContext, rwResources, context.m_pResidents->m_renders.m_opaqueStaticCount, m_currentFrame);

			// 2. BEGINS RENDERING
			ClearWindow(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight, m_swapchainBackBuffers[m_swapchainIDX].Get(), m_descriptorContext, m_swapchainIDX);

			// 3. TAKES THE COMMNANDS FROM THE DRAW CULL SHADER AND DRAWS THE SCENE
			// Shaders used: opaqueDraw.vs.hlsl + opaqueDraw.ps.hlsl
			DrawIndirect(cmdContext.m_graphicsCmdList.Get(), m_pipelineContext, m_descriptorContext, m_roResources, rwResources, m_currentFrame);
		}

		// Prepares transform buffer for next frame data copy
		D3D12_RESOURCE_BARRIER transformCopyBarrier{};
		CreateResourcesTransitionBarrier(transformCopyBarrier, rwResources.m_transformBuffer.m_ssbo.buffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &transformCopyBarrier);
		
#if defined(DX12_OCCLUSION_DRAW_CULL) && defined(BLIT_DEPTH_PYRAMID_TEST)

		// Depth pyramid debugging
		CopyDepthPyramidToSwapchain(frameTools.mainGraphicsCommandList.Get(), m_swapchainBackBuffers[swapchainIndex].Get(), varBuffers.depthPyramid.pyramid.Get(), 
			varBuffers.depthPyramid.width, varBuffers.depthPyramid.height, nullptr, m_commandQueue.Get(), m_swapchain.Get(), context.pCamera->transformData.debugPyramidLevel, 
			m_swapchainWidth, m_swapchainHeight);

#else
			
		D3D12_RESOURCE_BARRIER presentBarrier{};
		CreateResourcesTransitionBarrier(presentBarrier, m_swapchainBackBuffers[m_swapchainIDX].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &presentBarrier);

		cmdContext.m_graphicsCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_graphicsCmdList.Get()};
		m_commandQueue->ExecuteCommandLists(1, commandLists);
		
#endif
    }

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