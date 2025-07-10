#if defined(_WIN32)
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/Cull/dx12Cull.h"
#include "dx12RuntimeHelpers.h"
#include "Core/blitzenEngine.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenEngine
{
	void EndGPUCommands(BlitzenDX12::Dx12Renderer* pRenderer, BMPR_COMMAND_LIST_TYPE type)
	{
		auto& cmd{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };

		switch(type)
		{
		case BMPR_COMMAND_LIST_TYPE::GRAPHICS:
		{
			cmd.m_graphicsCmdList->Close();
			ID3D12CommandList* commandLists[] = { cmd.m_graphicsCmdList.Get() };
			pRenderer->m_commandQueue->ExecuteCommandLists(1, commandLists);
			break;
		}
		case BMPR_COMMAND_LIST_TYPE::COMPUTE:
		{
			cmd.m_computeCmdList->Close();
			ID3D12CommandList* commandLists[] = { cmd.m_computeCmdList.Get() };
			pRenderer->m_computeCommandQueue->ExecuteCommandLists(1, commandLists);
			break;
		}
		case BMPR_COMMAND_LIST_TYPE::TRANSFER:
		{
			cmd.m_copyCmdList->Close();
			ID3D12CommandList* commandLists[] = { cmd.m_copyCmdList.Get() };
			pRenderer->m_transferCommandQueue->ExecuteCommandLists(1, commandLists);
			break;
		}
		default:
		{
			BLIT_ERROR("%s: Unknown command list type in EndGPUCommands!", BlitzenCore::CE_DX12_SYSTEM_NAME);
			break;
		}
		}
		
	}

	void BeginGPUCommands(BlitzenDX12::Dx12Renderer* pRenderer, BMPR_COMMAND_LIST_TYPE type)
	{
		auto& cmd{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };
		if (type == BMPR_COMMAND_LIST_TYPE::GRAPHICS)
		{
			cmd.m_graphicsCmdAlloc->Reset();
			cmd.m_graphicsCmdList->Reset(cmd.m_graphicsCmdAlloc.Get(), nullptr);

			ID3D12DescriptorHeap* descriptorHeaps[]{ pRenderer->m_descriptorContext.m_viewHeap.Get(), pRenderer->m_descriptorContext.m_samplerHeap.Get()};
			cmd.m_graphicsCmdList->SetDescriptorHeaps(2, descriptorHeaps);

			cmd.m_graphicsCmdList->SetGraphicsRootSignature(pRenderer->m_pipelineContext.m_graphicsRoot.Get());
		}
		else if (type == BMPR_COMMAND_LIST_TYPE::COMPUTE)
		{
			cmd.m_computeCmdAlloc->Reset();
			cmd.m_computeCmdList->Reset(cmd.m_computeCmdAlloc.Get(), nullptr);

			ID3D12DescriptorHeap* descriptorHeaps[]{ pRenderer->m_descriptorContext.m_viewHeap.Get()};
			cmd.m_computeCmdList->SetDescriptorHeaps(1, descriptorHeaps);

			cmd.m_computeCmdList->SetComputeRootSignature(pRenderer->m_pipelineContext.m_cullRoot.Get());
		}
		else if (type == BMPR_COMMAND_LIST_TYPE::TRANSFER)
		{
			cmd.m_copyCmdAlloc->Reset();
			cmd.m_copyCmdList->Reset(cmd.m_copyCmdAlloc.Get(), nullptr);
		}
	}	

	void PresentRender(BlitzenDX12::Dx12Renderer* pRenderer, uint32_t waitCount)
	{
		pRenderer->m_swapchain->Present(1, 0);

		pRenderer->m_currentFrame = (pRenderer->m_currentFrame + 1) % BlitzenDX12::ce_framesInFlight;
	}

	void PlaceRendererFence(BlitzenDX12::Dx12Renderer* pRenderer, RENDERER_FENCE_TYPE type)
	{
		auto& cmd{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };

		switch (type)
		{
		case RENDERER_FENCE_TYPE::GRAPHICS:
		{
			BlitzenDX12::PlaceFence(cmd.m_frameFence.m_value, pRenderer->m_commandQueue.Get(), cmd.m_frameFence.m_dx12Handle.Get(), cmd.m_frameFence.m_event);
			break;
		}
		case RENDERER_FENCE_TYPE::COMPUTE:
		{
			BlitzenDX12::PlaceFence(cmd.m_computeFence.m_value, pRenderer->m_computeCommandQueue.Get(), cmd.m_computeFence.m_dx12Handle.Get(), cmd.m_computeFence.m_event);
			break;
		}
		case RENDERER_FENCE_TYPE::TRANSFER:
		{
			BlitzenDX12::PlaceFence(cmd.m_copyFence.m_value, pRenderer->m_transferCommandQueue.Get(), cmd.m_copyFence.m_dx12Handle.Get(), cmd.m_copyFence.m_event);
			break;
		}
		}
	}

	void UpdateRendererView(BlitzenDX12::Dx12Renderer* pRenderer, CameraViewData& viewData, bool isFrustumFrozen)
	{
		auto& cmd{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };
		auto& rwResources{ pRenderer->m_rwResources[pRenderer->m_currentFrame] };
		auto& pipelineContext{ pRenderer->m_pipelineContext };
		auto& descriptorContext{ pRenderer->m_descriptorContext };
		UINT frame{ pRenderer->m_currentFrame };

		if (isFrustumFrozen)
		{
			// Only change the matrix that moves the camera if the freeze frustum debug functionality is active
			rwResources.m_viewBuffer.pData->projectionViewMatrix = viewData.projectionViewMatrix;
		}
		else
		{
			BlitzenCore::BlitMemCopy(rwResources.m_viewBuffer.pData, &viewData, sizeof(CameraViewData));
		}
	}

	void UpdateRendererTransforms(BlitzenDX12::Dx12Renderer* pRenderer, BlitzenEngine::CPU_TRANSFORM* pTransforms, uint32_t transformCount)
	{
		auto& cmd{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };
		auto& rwResources{ pRenderer->m_rwResources[pRenderer->m_currentFrame] };

		BlitzenCore::BlitMemCopy(pRenderer->m_roResources.CPU_MOVING_OBJECT_BUFFER.m_pMapped, pTransforms, transformCount * sizeof(CPU_TRANSFORM));
		
		cmd.m_copyCmdList->CopyBufferRegion(rwResources.m_movementBuffer.buffer.Get(), 0, pRenderer->m_roResources.CPU_MOVING_OBJECT_BUFFER.m_buffer.Get(), 0,
			transformCount * sizeof(CPU_TRANSFORM));
		
		cmd.m_copyCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmd.m_copyCmdList.Get() };
		pRenderer->m_transferCommandQueue->ExecuteCommandLists(1, commandLists);
		
		// Fence until transforms are ready
		BlitzenDX12::PlaceFence(cmd.m_copyFence.m_value, pRenderer->m_transferCommandQueue.Get(), cmd.m_copyFence.m_dx12Handle.Get(), cmd.m_copyFence.m_event);
	}

	void ChangeCullingBuffersToReadbackMode(RendererPtrType pRenderer)
	{
		D3D12_RESOURCE_BARRIER resourceBarriers[1]{};
		BlitzenDX12::CreateResourcesTransitionBarrier(resourceBarriers[0], pRenderer->m_rwResources[pRenderer->m_currentFrame].m_movementBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pRenderer->m_cmdContext[pRenderer->m_currentFrame].m_computeCmdList->ResourceBarrier(BLIT_ARRAY_SIZE(resourceBarriers), resourceBarriers);
	}

	void RequestGameLogicUpdatesFromShader(BlitzenDX12::Dx12Renderer* pRenderer, SHADER_GAME_LOGIC_UPDATES& outUpdate)
	{
		auto& cmd{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };
		auto& rwResources{ pRenderer->m_rwResources[pRenderer->m_currentFrame] };
		auto& roResources{ pRenderer->m_roResources };

		cmd.m_copyCmdAlloc->Reset();
		cmd.m_copyCmdList->Reset(cmd.m_copyCmdAlloc.Get(), nullptr);

		cmd.m_copyCmdList->CopyBufferRegion(pRenderer->m_roResources.GPU_MOVING_OBJECT_READBACK.m_buffer.Get(), 0, rwResources.m_movementBuffer.buffer.Get(), 0,
			outUpdate.m_transformCount * sizeof(MeshTransform));

		cmd.m_copyCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmd.m_copyCmdList.Get() };
		pRenderer->m_transferCommandQueue->ExecuteCommandLists(1, commandLists);

		// Fence until transforms are ready
		BlitzenDX12::PlaceFence(cmd.m_copyFence.m_value, pRenderer->m_transferCommandQueue.Get(), cmd.m_copyFence.m_dx12Handle.Get(), cmd.m_copyFence.m_event);

		BeginGPUCommands(pRenderer, BMPR_COMMAND_LIST_TYPE::COMPUTE);

		D3D12_RESOURCE_BARRIER resourceBarriers[1]{};
		BlitzenDX12::CreateResourcesTransitionBarrier(resourceBarriers[0], rwResources.m_movementBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		cmd.m_computeCmdList->ResourceBarrier(BLIT_ARRAY_SIZE(resourceBarriers), resourceBarriers);

		BlitzenCore::BlitMemCopy(outUpdate.pGpuTransorms, pRenderer->m_roResources.GPU_MOVING_OBJECT_READBACK.m_pMapped, outUpdate.m_transformCount * sizeof(CPU_TRANSFORM));
	}

	BlitML::vec2 UpdateRendererWindowData(BlitzenDX12::Dx12Renderer* pRenderer, uint32_t newWidth, uint32_t newHeight, BlitzenPlatform::PlatformContext* pbpHandle)
	{
		auto& rwResources = pRenderer->m_rwResources[pRenderer->m_currentFrame];

		pRenderer->m_swapchainWidth = newWidth;
		pRenderer->m_swapchainHeight = newHeight;

		HWND hwnd{ pbpHandle->m_hwnd };
		BlitzenDX12::RecreateSwapchain(hwnd, pRenderer->m_factory.Get(), pRenderer->m_device.Get(), pRenderer->m_commandQueue.Get(), newWidth, newHeight, &pRenderer->m_swapchain,
			pRenderer->m_swapchainBackBuffers, pRenderer->m_depthBuffers, pRenderer->m_descriptorContext, pRenderer->m_cmdContext);

		BlitzenDX12::CreateOMSTargetDescs(pRenderer->m_pipelineContext.m_renderTargetPassDesc, pRenderer->m_pipelineContext.m_depthTargetPassDesc, 
			pRenderer->m_descriptorContext.m_swapchainRtvHandle, pRenderer->m_descriptorContext.m_depthTargetDSVHandle);

		if constexpr (BlitzenCore::Ce_Build_HI_Z)
		{
			for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
			{
				BlitzenDX12::CreateDepthPyramidResource(pRenderer->m_device.Get(), pRenderer->m_rwResources[i].m_HI_Z, pRenderer->m_swapchainWidth, pRenderer->m_swapchainHeight);
			}

			BlitzenDX12::RecreateDepthPyramidDescriptors(pRenderer->m_device.Get(), pRenderer->m_rwResources, pRenderer->m_descriptorContext, pRenderer->m_swapchainWidth, pRenderer->m_swapchainHeight);

			BlitzenDX12::RecreateDepthTargetDescriptor(pRenderer->m_device.Get(), pRenderer->m_depthBuffers, pRenderer->m_descriptorContext);
		}

		return BlitML::vec2{ float(rwResources.m_HI_Z.width), float(rwResources.m_HI_Z.height) };
	}

	void PrepareRendererForRuntime(BlitzenDX12::Dx12Renderer* pRenderer)
	{
		auto& cmdContext{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };
		auto rwResources{ pRenderer->m_rwResources };
		auto& roResources{ pRenderer->m_roResources };

		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), nullptr);

		// READ ONLY BARRIERS
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> staticBufferBarriers{ pRenderer->m_roResources.BUFFER_COUNT, {} };

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_VtxPosStagingBufferIndex], roResources.m_vtxPosBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_VtxNrmStagingBufferIndex], roResources.m_vtxNrmBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_VtxTangentsStagingBufferIndex], roResources.m_vtxTangentBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_VtxTexCoordStagingBufferIndex], roResources.m_vtxTexCoordBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_IndexStagingBufferIndex], roResources.m_idxBuffer.m_buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_SurfaceStagingBufferIndex], roResources.m_surfaceBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_RenderStagingBufferIndex], roResources.m_renderBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_LodStagingIndex], roResources.m_LODBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_MaterialStagingIndex], roResources.m_matBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_BoundingSphereBoundingIndex], roResources.m_boundingSpheres.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::CE_TERRAIN_VERTEX_SSBO_STAGING_IDX], roResources.m_terrainVtxBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::CE_TERRAIN_VTX_IDX_SSBO_STAGING_IDX], roResources.m_terrainIdxBuffer.m_buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		if (BlitzenCore::Ce_BuildClusters)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_ClusterVtxsStagingIndex], pRenderer->m_roResources.m_clusterVtxsBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_ClusterSpheresStagingIndex], pRenderer->m_roResources.m_clusterSpheresBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_ClusterConesStagingIndex], pRenderer->m_roResources.m_clusterConesBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		// EXECUTE
		cmdContext.m_graphicsCmdList->ResourceBarrier(pRenderer->m_roResources.BUFFER_COUNT, staticBufferBarriers.Data());

		D3D12_RESOURCE_BARRIER CPU_COMMUNICATION_BUFFERS[2]{};
		BlitzenDX12::CreateResourcesTransitionBarrier(CPU_COMMUNICATION_BUFFERS[0], roResources.CPU_MOVING_OBJECT_BUFFER.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		BlitzenDX12::CreateResourcesTransitionBarrier(CPU_COMMUNICATION_BUFFERS[1], roResources.GPU_MOVING_OBJECT_READBACK.m_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		cmdContext.m_graphicsCmdList->ResourceBarrier(BLIT_ARRAY_SIZE(CPU_COMMUNICATION_BUFFERS), CPU_COMMUNICATION_BUFFERS);

		constexpr uint32_t CE_RW_BUFFER_INITIAL_COUNT = 7 * BlitzenDX12::ce_framesInFlight;

		// RW BUFFERS
		uint32_t rwID{ 0 };
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> rwBuffersFinal{ CE_RW_BUFFER_INITIAL_COUNT };

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], rwResources[i].m_viewBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
			rwID++;
		}

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], rwResources[i].m_transformBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			rwID++;
		}

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], rwResources[i].m_movementBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			rwID++;
		}

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], pRenderer->m_rwResources[i].m_staticDrawCmdBuffer.buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			rwID++;
		}

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], pRenderer->m_rwResources[i].m_staticDrawCmdCounter.buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			rwID++;
		}

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], pRenderer->m_rwResources[i].m_dynamicDrawCmdBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			rwID++;
		}

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], pRenderer->m_rwResources[i].m_dynamicDrawCmdCounter.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			rwID++;
		}

		BLIT_ASSERT(rwID == CE_RW_BUFFER_INITIAL_COUNT);

		if constexpr (BlitzenCore::Ce_InstanceCulling)
		{
			
		}

		if constexpr (BlitzenCore::Ce_OcclusionCulling)
		{

			for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
			{
				D3D12_RESOURCE_BARRIER drawVisibilityBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(drawVisibilityBarrier, pRenderer->m_rwResources[i].m_drawVisBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				rwBuffersFinal.PushBack(drawVisibilityBarrier);
			}
		}

		if constexpr (BlitzenDX12::CE_DX12_BUILD_HI_Z_MAP)
		{
			for (uint32_t f = 0; f < BlitzenDX12::ce_framesInFlight; ++f)
			{
				for (uint32_t hi_z_mip = 0; hi_z_mip < pRenderer->m_rwResources[f].m_HI_Z.mipCount; ++hi_z_mip)
				{
					D3D12_RESOURCE_BARRIER depthPyramidBarrier{};
					BlitzenDX12::CreateResourcesTransitionBarrier(depthPyramidBarrier, pRenderer->m_rwResources[f].m_HI_Z.pyramid.Get(), 
						D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, hi_z_mip);
					rwBuffersFinal.PushBack(depthPyramidBarrier);
				}
			}

			for (uint32_t f = 0; f < BlitzenDX12::ce_framesInFlight; ++f)
			{
				D3D12_RESOURCE_BARRIER depthTargetBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(depthTargetBarrier, pRenderer->m_depthBuffers[f].Get(),
					D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				rwBuffersFinal.PushBack(depthTargetBarrier);
			}
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			for (uint32_t frame = 0; frame < BlitzenDX12::ce_framesInFlight; ++frame)
			{
				auto& rwResources{ pRenderer->m_rwResources[frame] };

				D3D12_RESOURCE_BARRIER clusterDispatchBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterDispatchBarrier, rwResources.m_clusterGroupCounter.buffer.Get(), 
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

				rwBuffersFinal.PushBack(clusterDispatchBarrier);

				D3D12_RESOURCE_BARRIER clusterDispatchCounterBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterDispatchCounterBarrier, rwResources.m_clusterVisibilityBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				rwBuffersFinal.PushBack(clusterDispatchCounterBarrier);

				D3D12_RESOURCE_BARRIER clusterGroupDataBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterGroupDataBarrier, rwResources.m_clusterGroupDataBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				rwBuffersFinal.PushBack(clusterGroupDataBarrier);
			}
		}

		// EXECUTE
		cmdContext.m_graphicsCmdList->ResourceBarrier((UINT)rwBuffersFinal.GetSize(), rwBuffersFinal.Data());

		// TEXTURES
		for (uint32_t i = 1; i < pRenderer->m_roResources.m_textureCount; ++i)
		{
			D3D12_RESOURCE_BARRIER textureFinalBarrier{};
			BlitzenDX12::CreateResourcesTransitionBarrier(textureFinalBarrier, pRenderer->m_roResources.m_drawTextures[i].resource.Get(), 
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &textureFinalBarrier);
		}

		cmdContext.m_graphicsCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_graphicsCmdList.Get() };
		pRenderer->m_commandQueue->ExecuteCommandLists(1, commandLists);

		BlitzenDX12::PlaceFence(cmdContext.m_frameFence.m_value, pRenderer->m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), 
			cmdContext.m_frameFence.m_event);
		BlitzenDX12::PlaceFence(cmdContext.m_copyFence.m_value, pRenderer->m_transferCommandQueue.Get(), cmdContext.m_copyFence.m_dx12Handle.Get(),
			cmdContext.m_copyFence.m_event);
	}
}

#endif