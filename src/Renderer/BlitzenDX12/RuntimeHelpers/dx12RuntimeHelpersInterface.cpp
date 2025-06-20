#if defined(_WIN32)

#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Core/blitzenEngine.h"

namespace BlitzenEngine
{
	void PrepareRendererForRuntime(BlitzenDX12::Dx12Renderer* pRenderer)
	{
		auto& cmdContext{ pRenderer->m_cmdContext[pRenderer->m_currentFrame] };

		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), nullptr);

		// READ ONLY BARRIERS
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> staticBufferBarriers{ pRenderer->m_roResources.BUFFER_COUNT, {} };

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_VtxPosStagingBufferIndex], pRenderer->m_roResources.m_vtxPosBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_VtxNrmStagingBufferIndex], pRenderer->m_roResources.m_vtxNrmBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_VtxTangentsStagingBufferIndex], pRenderer->m_roResources.m_vtxTangentBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_VtxTexCoordStagingBufferIndex], pRenderer->m_roResources.m_vtxTexCoordBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_IndexStagingBufferIndex], pRenderer->m_roResources.m_idxBuffer.m_buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_SurfaceStagingBufferIndex], pRenderer->m_roResources.m_surfaceBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_RenderStagingBufferIndex], pRenderer->m_roResources.m_renderBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_LodStagingIndex], pRenderer->m_roResources.m_LODBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		BlitzenDX12::CreateResourcesTransitionBarrier(staticBufferBarriers[BlitzenDX12::Ce_MaterialStagingIndex], pRenderer->m_roResources.m_matBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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

		// RW BUFFERS
		uint32_t rwID{ 0 };
		BlitCL::DynamicArray<D3D12_RESOURCE_BARRIER> rwBuffersFinal{ BlitzenDX12::Ce_VarBuffersCount };

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], pRenderer->m_rwResources[i].m_viewBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ);
			rwID++;
		}

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			// Starts off as indirect argument, because the first transition barrier will be expecting that
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], pRenderer->m_rwResources[i].m_drawCmdBuffer.buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			rwID++;
		}

		for (uint32_t i = 0; i < BlitzenDX12::ce_framesInFlight; ++i)
		{
			// Starts off as indirect argument, because the first transition barrier will be expecting that
			BlitzenDX12::CreateResourcesTransitionBarrier(rwBuffersFinal[rwID], pRenderer->m_rwResources[i].m_drawCmdCounterBuffer.buffer.Get(), 
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			rwID++;
		}

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
		}

		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			for (uint32_t frame = 0; frame < BlitzenDX12::ce_framesInFlight; ++frame)
			{
				auto& rwResources{ pRenderer->m_rwResources[frame] };

				D3D12_RESOURCE_BARRIER clusterDispatchBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterDispatchBarrier, rwResources.m_clusterDispatchBuffer.buffer.Get(), 
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
		for (uint32_t i = 0; i < pRenderer->m_roResources.m_textureCount; ++i)
		{
			D3D12_RESOURCE_BARRIER textureFinalBarrier{};
			BlitzenDX12::CreateResourcesTransitionBarrier(textureFinalBarrier, pRenderer->m_roResources.m_drawTextures[i].resource.Get(), 
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			cmdContext.m_graphicsCmdList->ResourceBarrier(1, &textureFinalBarrier);
		}

		cmdContext.m_graphicsCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_graphicsCmdList.Get() };
		pRenderer->m_commandQueue->ExecuteCommandLists(1, commandLists);

		BlitzenDX12::PlaceFence(cmdContext.m_frameFence.m_value, pRenderer->m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);
	}
}

#endif