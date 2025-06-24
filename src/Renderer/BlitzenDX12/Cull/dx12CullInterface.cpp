#if defined(_WIN32)
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/Cull/dx12Cull.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
	void DispatchCullingShaders(BlitzenDX12::Dx12Renderer* pRenderer, const CULL_CONTEXT& cullContext)
	{
		UINT frame{ pRenderer->m_currentFrame };
		auto commandList = pRenderer->m_cmdContext[frame].m_graphicsCmdList.Get();
		auto& rwResources{ pRenderer->m_rwResources[frame] };
		auto& pipelineContext{ pRenderer->m_pipelineContext };
		auto& descriptorContext{ pRenderer->m_descriptorContext };

		switch (cullContext.m_cullType)
		{
			case BLIT_CULL_TYPE::DRAW_CULL_DEFAULT:
			{
				switch (cullContext.m_workType)
				{
				/*************************************************************************************************************************************************
				* FRUSTUM CULLING + LOD SELECTION ON OPAQUE STATIC OBJECTS																						 *
				**************************************************************************************************************************************************/
				case RENDER_OBJECT_TYPE::OPAQUE_STATIC:
				{
					// Binds heap for compute
					ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get() };
					commandList->SetDescriptorHeaps(1, srvHeaps);

					// Resets Count
					BlitzenDX12::DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), 
						descriptorContext.m_drawCullViewsHandle[frame], rwResources);

					// Culling barrier, waits for draw count reset and draw command read
					D3D12_RESOURCE_BARRIER cullingBarriers[2]{};
					// Count reset barrier 
					BlitzenDX12::CreateResourceUAVBarrier(cullingBarriers[0], rwResources.m_drawCmdCounterBuffer.buffer.Get());
					// Command read barrier
					BlitzenDX12::CreateResourcesTransitionBarrier(cullingBarriers[1], rwResources.m_drawCmdBuffer.buffer.Get(), 
						D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					// execute
					commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

					// Descriptors
					commandList->SetComputeRootSignature(pipelineContext.m_drawCullRoot.Get());
					commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_DrawCullSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
					commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_DrawCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);

					// Pipeline + constants
					commandList->SetPipelineState(pipelineContext.m_drawCullPso.Get());
					commandList->SetComputeRoot32BitConstant(BlitzenDX12::Ce_DrawCullDrawCountConstantRootID, cullContext.m_workCount, 0);

					// CULL
					commandList->Dispatch(BlitML::GetComputeShaderGroupSize(cullContext.m_workCount, 64), 1, 1);

					// Block graphics, should wait for command and count write
					D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
					// Command write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), 
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					// Counter write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), 
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					// execute
					commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);

					break;
				}
				default:
				{
					break;
				}
				}
				break;
			}
			case BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION:
			{
				switch (cullContext.m_workType)
				{
				/*************************************************************************************************************************************************
				* FRUSTUM CULLING + OCCLUSION CULLING(last frame Hierarchical Z Map) + LOD SELECTION ON OPAQUE STATIC OBJECTS									 *
				**************************************************************************************************************************************************/
				case RENDER_OBJECT_TYPE::OPAQUE_STATIC:
				{
					// Binds heap for compute
					ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get() };
					commandList->SetDescriptorHeaps(1, srvHeaps);

					// Draw count reset
					DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

					// Blocks culling shader, waits for indirect command read and count reset(the depth pyramid has a barrier in its generate function)
					D3D12_RESOURCE_BARRIER cullingBarriers[2]{};
					// Command ssbo
					BlitzenDX12::CreateResourcesTransitionBarrier(cullingBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(),
						D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					// Count reset
					BlitzenDX12::CreateResourceUAVBarrier(cullingBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get());
					// execute
					commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

					// Descriptors
					commandList->SetComputeRootSignature(pipelineContext.m_drawOccLateRoot.Get());
					commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_DrawOccTemporalSharedSRVsRootId, descriptorContext.m_sharedViewHandle[frame]);
					commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_DrawOccTemporalExclusiveSRVsRootId, descriptorContext.m_drawCullViewsHandle[frame]);
					commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_DrawOccTemporalHI_Z_MapRootId, descriptorContext.m_HI_Z_MapSRVHandle[frame]);

					// Pipeline + root constants
					commandList->SetPipelineState(pipelineContext.m_drawOccTemporalPso.Get());
					commandList->SetComputeRoot32BitConstant(BlitzenDX12::Ce_DrawCullDrawCountConstantRootID, cullContext.m_workCount, 0);

					// CULL
					commandList->Dispatch(BlitML::GetComputeShaderGroupSize(cullContext.m_workCount, 64), 1, 1);

					// Block graphics, should wait for command and count write
					D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
					// command write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(),
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					// count write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(),
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					// execute
					commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
					break;
				}
				case RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC:
				{
					break;
				}
				default:
				{
					break;
				}
				}
				break;
			}
			/**************************************************************************************************************************************************
			* FRUSTUM CULLING + OCCLUSION CULLING(last frame Hierarchical Z Map) + LOD SELECTION ON OPAQUE STATIC OBJECTS									  *
			* THE RESULTS ARE USED TO DO ANOTHER CULLING PASS ON THE VISIBLE OBJECT CLUSTERS																  *	
			* FINALLY, ONE LAST COMPUTE PASS IS DONE TO BATCH THE VISIBLE CLUSTERS																			  *
			***************************************************************************************************************************************************/
			case BLIT_CULL_TYPE::CLUSTER_CULL_DEFAULT:
			{
				ID3D12DescriptorHeap* srvHeaps[]{ descriptorContext.m_viewHeap.Get() };
				commandList->SetDescriptorHeaps(1, srvHeaps);

				/*
					DRAW CULLING PROCESS
				*/
				// Unordered access counter
				D3D12_RESOURCE_BARRIER dispatchResetBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(dispatchResetBarrier, rwResources.m_clusterDispatchBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				commandList->ResourceBarrier(1, &dispatchResetBarrier);

				// Descriptors
				commandList->SetComputeRootSignature(pipelineContext.m_clusterCullRoot.Get());
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);

				// Pipeline
				commandList->SetPipelineState(pipelineContext.m_clusterCullCmdResetPso.Get());

				// DISPATCH COUNTER RESET
				commandList->Dispatch(1, 1, 1);

				// Blocks dispatch, waits for cluster dispatch counter reset and dispatch cmd read
				D3D12_RESOURCE_BARRIER clusterDispatchBarriers[2]{};
				// dispatch counter reset
				BlitzenDX12::CreateResourceUAVBarrier(clusterDispatchBarriers[0], rwResources.m_clusterDispatchBuffer.buffer.Get());
				// dispatch group data read
				BlitzenDX12::CreateResourceUAVBarrier(clusterDispatchBarriers[1], rwResources.m_clusterGroupDataBuffer.buffer.Get());
				// execute
				commandList->ResourceBarrier(BLIT_ARRAY_SIZE(clusterDispatchBarriers), clusterDispatchBarriers);

				// Descriptors
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullHI_Z_MapSrvRootID, descriptorContext.m_HI_Z_MapSRVHandle[frame]);

				// Pipelines and constants
				commandList->SetPipelineState(pipelineContext.m_clusterCullDispatchPso.Get());
				commandList->SetComputeRoot32BitConstant(BlitzenDX12::Ce_ClusterCullDrawCountRootID, cullContext.m_workCount, 0);

				// CULL DRAWS
				commandList->Dispatch(BlitML::GetComputeShaderGroupSize(cullContext.m_workCount, 64), 1, 1);

				/*
					CLUSTER CULLING PROCESS
				*/
				// Blocks culling, waits for cluster dispatch count write and cluster dispatch cmd write, draw cmd read and draw cmd count reset
				D3D12_RESOURCE_BARRIER clusterCullBarriers[3]{};
				// cluster dispatch count
				BlitzenDX12::CreateResourceUAVBarrier(clusterCullBarriers[0], rwResources.m_clusterGroupDataBuffer.buffer.Get());
				// cluster dispatch cmd
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterCullBarriers[1], rwResources.m_clusterDispatchBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				// draw cmd count reset
				BlitzenDX12::CreateResourceUAVBarrier(clusterCullBarriers[2], rwResources.m_clusterVisibilityBuffer.buffer.Get());
				// execute
				commandList->ResourceBarrier(BLIT_ARRAY_SIZE(clusterCullBarriers), clusterCullBarriers);

				// descriptors
				commandList->SetComputeRootSignature(pipelineContext.m_clusterCullRoot.Get());
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullHI_Z_MapSrvRootID, descriptorContext.m_HI_Z_MapSRVHandle[frame]);

				commandList->SetPipelineState(pipelineContext.m_clusterCullPso.Get());

				// CULL CLUSTERS
				commandList->ExecuteIndirect(pipelineContext.m_clusterCullCmdSign.Get(), 1, rwResources.m_clusterDispatchBuffer.buffer.Get(), 0, nullptr, 0);

				/*
					CLUSTER BATCHING PROCESS
				*/
				DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

				D3D12_RESOURCE_BARRIER clusterBatchCmdBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterBatchCmdBarrier, rwResources.m_clusterDispatchBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				commandList->ResourceBarrier(1, &clusterBatchCmdBarrier);

				commandList->SetComputeRootSignature(pipelineContext.m_clusterCullRoot.Get());
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);

				commandList->SetPipelineState(pipelineContext.m_clusterCullBatchCmdPso.Get());
				commandList->Dispatch(1, 1, 1);

				D3D12_RESOURCE_BARRIER clusterBatchBarriers[5]{};
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterBatchBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				BlitzenDX12::CreateResourceUAVBarrier(clusterBatchBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get());
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterBatchBarriers[2], rwResources.m_clusterDispatchBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				BlitzenDX12::CreateResourceUAVBarrier(clusterBatchBarriers[3], rwResources.m_clusterVisibilityBuffer.buffer.Get());
				BlitzenDX12::CreateResourceUAVBarrier(clusterBatchBarriers[4], rwResources.m_clusterGroupDataBuffer.buffer.Get());
				commandList->ResourceBarrier(BLIT_ARRAY_SIZE(clusterBatchBarriers), clusterBatchBarriers);

				// descriptors
				commandList->SetComputeRootSignature(pipelineContext.m_clusterCullRoot.Get());
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_ClusterCullAdditionalViewsRootID, descriptorContext.m_clusterDispatchAdditionalUAVsHandle[frame]);

				commandList->SetPipelineState(pipelineContext.m_clusterCullBatchPso.Get());

				// BATCH CLUSTERS
				commandList->ExecuteIndirect(pipelineContext.m_clusterCullCmdSign.Get(), 1, rwResources.m_clusterDispatchBuffer.buffer.Get(), 0, nullptr, 0);

				// Block graphics, should wait for command and count write
				D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
				// command write
				BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				// count write
				BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				// execute
				commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
				break;
			}
			default:
			{
				BLIT_ASSERT_MESSAGE(true, "Requested suspended culling type");
			}
		}
	}

	void GenerateHI_Z_MAP(BlitzenDX12::Dx12Renderer* pRenderer)
	{
		UINT frame{ pRenderer->m_currentFrame };
		UINT swapchainId{ pRenderer->m_swapchainIDX };
		auto commandList{ pRenderer->m_cmdContext[frame].m_graphicsCmdList.Get() };
		auto depthTarget{ pRenderer->m_depthBuffers[swapchainId].Get() };
		auto& rwResources{ pRenderer->m_rwResources[frame] };
		auto& pipelineContext{ pRenderer->m_pipelineContext };
		auto& descriptorContext{ pRenderer->m_descriptorContext };

		// Binds heap for compute
		ID3D12DescriptorHeap* heaps[] = { descriptorContext.m_viewHeap.Get() };
		commandList->SetDescriptorHeaps(1, heaps);

		// Barrier for depth pyramid generation, waits for depth target write and HI Z map read
		D3D12_RESOURCE_BARRIER depthPyramidBarriers[2]{};
		// Depth target write
		BlitzenDX12::CreateResourcesTransitionBarrier(depthPyramidBarriers[0], depthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		// HI Z map read
		BlitzenDX12::CreateResourcesTransitionBarrier(depthPyramidBarriers[1], rwResources.m_HI_Z.pyramid.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
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

			commandList->SetComputeRoot32BitConstant(BlitzenDX12::Ce_HI_Z_MapMipLvlConstantRootID, mipLevel, 0);

			// Binds write texture (the depth pyramid has a copy for double buffering and each one has the correct offsets for the descriptor heap)
			commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_HI_Z_MapUAVRootID, rwResources.m_HI_Z.mips[i]);

			// Binds read texture (For first level, it's the depth target. For every other level, it's the depth pyramid itself)
			if (i == 0)
			{
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_HI_Z_MapSRVRootID, descriptorContext.m_depthTargetSRVHandle[swapchainId]);
			}
			else
			{
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::Ce_HI_Z_MapSRVRootID, descriptorContext.m_HI_Z_MapSRVHandle[frame]);
				mipLevel++;
			}

			// Generate level
			commandList->Dispatch(BlitML::GetComputeShaderGroupSize(levelWidth, 32), BlitML::GetComputeShaderGroupSize(levelHeight, 32), 1);

			// Barrier for the next loop, since it will use the current mip as the read descriptor
			D3D12_RESOURCE_BARRIER nextLoopBarrier{};
			BlitzenDX12::CreateResourceUAVBarrier(nextLoopBarrier, rwResources.m_HI_Z.pyramid.Get());
			commandList->ResourceBarrier(1, &nextLoopBarrier);
		}

		// Culling waits for hi z write
		D3D12_RESOURCE_BARRIER cullingBarrier{};
		BlitzenDX12::CreateResourcesTransitionBarrier(cullingBarrier, rwResources.m_HI_Z.pyramid.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &cullingBarrier);

		// Graphics wait for depth target read
		D3D12_RESOURCE_BARRIER graphicsBarrier{};
		BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarrier, depthTarget, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandList->ResourceBarrier(1, &graphicsBarrier);
	}
}

#endif