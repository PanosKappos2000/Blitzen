#if defined(_WIN32)

// BLITZEN IS ABSOLUTE

#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/Cull/dx12Cull.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
	void DispatchCullingShaders(BlitzenDX12::Dx12Renderer* pRenderer, const CULL_CONTEXT& cullContext)
	{
		UINT frame{ pRenderer->m_currentFrame };
		auto commandList = pRenderer->m_cmdContext[frame].m_computeCmdList.Get();
		auto& rwResources{ pRenderer->m_rwResources[frame] };
		auto& pipelineContext{ pRenderer->m_pipelineContext };
		auto& descriptorContext{ pRenderer->m_descriptorContext };

		commandList->SetComputeRootSignature(pipelineContext.m_cullRoot.Get());
		commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_CULL_ROOT_GLOBAL_ID, descriptorContext.m_globalTableHandle[frame]);
		commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_CULL_ROOT_CULL_GLOBAL_ID, descriptorContext.m_cullGlobalTableHandle[frame]);

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
					// BARRIER ON COUNTER BEFORE RESET
					D3D12_RESOURCE_BARRIER resetBarrier{};
					BlitzenDX12::CreateResourcesTransitionBarrier(resetBarrier, rwResources.m_staticDrawCmdCounter.buffer.Get(), 
						D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					commandList->ResourceBarrier(1, &resetBarrier);

					// Descriptors
					commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_CULL_ROOT_STATIC_TABLE_ID, descriptorContext.m_cullOSTableHandle[frame]);

					// Pipeline + constants
					commandList->SetPipelineState(pipelineContext.m_opaqueStaticCountResetPso.Get());

					commandList->Dispatch(1, 1, 1);

					// Culling barrier, waits for draw count reset and draw command read
					D3D12_RESOURCE_BARRIER cullingBarriers[2]{};
					// Count reset barrier 
					BlitzenDX12::CreateResourceUAVBarrier(cullingBarriers[0], rwResources.m_staticDrawCmdCounter.buffer.Get());
					// Command read barrier
					BlitzenDX12::CreateResourcesTransitionBarrier(cullingBarriers[1], rwResources.m_staticDrawCmdBuffer.buffer.Get(), 
						D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					// execute
					commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

					// Pipeline + constants
					commandList->SetPipelineState(pipelineContext.m_staticCullPso.Get());
					commandList->SetComputeRoot32BitConstant(BlitzenDX12::CE_CULL_ROOT_STATIC_WORK_CONSTANT_ID, cullContext.m_workCount, 0);

					// CULL
					commandList->Dispatch(BlitML::GetComputeShaderGroupSize(cullContext.m_workCount, 64), 1, 1);

					// Block graphics, should wait for command and count write
					D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
					// Command write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_staticDrawCmdBuffer.buffer.Get(), 
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					// Counter write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_staticDrawCmdCounter.buffer.Get(), 
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
					// BARRIER ON COUNTER BEFORE RESET
					D3D12_RESOURCE_BARRIER resetBarrier{};
					BlitzenDX12::CreateResourcesTransitionBarrier(resetBarrier, rwResources.m_staticDrawCmdCounter.buffer.Get(),
						D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					commandList->ResourceBarrier(1, &resetBarrier);

					commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_CULL_ROOT_STATIC_TABLE_ID, descriptorContext.m_cullOSTableHandle[frame]);

					commandList->SetPipelineState(pipelineContext.m_opaqueStaticCountResetPso.Get());

					// RESET 
					commandList->Dispatch(1, 1, 1);

					// Culling barrier, waits for draw count reset and draw command read
					D3D12_RESOURCE_BARRIER cullingBarriers[2]{};
					// Count reset barrier 
					BlitzenDX12::CreateResourceUAVBarrier(cullingBarriers[0], rwResources.m_staticDrawCmdCounter.buffer.Get());
					// Command read barrier
					BlitzenDX12::CreateResourcesTransitionBarrier(cullingBarriers[1], rwResources.m_staticDrawCmdBuffer.buffer.Get(),
						D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					// execute
					commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

					commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_CULL_ROOT_HI_Z_MAP_ID, descriptorContext.m_HI_Z_MAP_cullHandle[frame]);

					// Pipeline + constants
					commandList->SetPipelineState(pipelineContext.m_drawOccTemporalPso.Get());
					commandList->SetComputeRoot32BitConstant(BlitzenDX12::CE_CULL_ROOT_STATIC_WORK_CONSTANT_ID, cullContext.m_workCount, 0);

					// CULL
					commandList->Dispatch(BlitML::GetComputeShaderGroupSize(cullContext.m_workCount, 64), 1, 1);

					// Block graphics, should wait for command and count write
					D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
					// Command write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_staticDrawCmdBuffer.buffer.Get(),
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					// Counter write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_staticDrawCmdCounter.buffer.Get(),
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					// execute
					commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
					break;
				}
				case RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC:
				{
					// BARRIER ON COUNTER BEFORE RESET
					D3D12_RESOURCE_BARRIER resetBarrier{};
					BlitzenDX12::CreateResourcesTransitionBarrier(resetBarrier, rwResources.m_dynamicDrawCmdCounter.buffer.Get(),
						D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					commandList->ResourceBarrier(1, &resetBarrier);

					commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_CULL_ROOT_DYNAMIC_TABLE_ID, descriptorContext.m_cullODTableHandle[frame]);

					commandList->SetPipelineState(pipelineContext.m_opaqueDynamicCountResetPso.Get());

					// RESET 
					commandList->Dispatch(1, 1, 1);

					// Culling barrier, waits for draw count reset and draw command read
					D3D12_RESOURCE_BARRIER cullingBarriers[4]{};
					// Count reset barrier 
					BlitzenDX12::CreateResourceUAVBarrier(cullingBarriers[0], rwResources.m_dynamicDrawCmdCounter.buffer.Get());
					// Command read barrier
					BlitzenDX12::CreateResourcesTransitionBarrier(cullingBarriers[1], rwResources.m_dynamicDrawCmdBuffer.buffer.Get(),
						D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					BlitzenDX12::CreateResourcesTransitionBarrier(cullingBarriers[2], pRenderer->MCpuLogicBuffers.GPUSSBOWorldVariableTransform.buffer.Get(),
						D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					BlitzenDX12::CreateResourceUAVBarrier(cullingBarriers[3], rwResources.m_transformBuffer.buffer.Get());
					// execute
					commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

					commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_CULL_ROOT_HI_Z_MAP_ID, descriptorContext.m_HI_Z_MAP_cullHandle[frame]);

					// Pipeline + constants
					commandList->SetPipelineState(pipelineContext.m_dynamicCullPso.Get());
					commandList->SetComputeRoot32BitConstant(BlitzenDX12::CE_CULL_ROOT_DYNAMIC_WORK_CONSTANT_ID, cullContext.m_workCount, 0);

					// CULL
					commandList->Dispatch(BlitML::GetComputeShaderGroupSize(cullContext.m_workCount, 64), 1, 1);

					// Block graphics, should wait for command and count write
					D3D12_RESOURCE_BARRIER graphicsBarriers[3]{};
					// Command write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_dynamicDrawCmdBuffer.buffer.Get(),
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					// Counter write
					BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_dynamicDrawCmdCounter.buffer.Get(),
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
					BlitzenDX12::CreateResourceUAVBarrier(graphicsBarriers[2], rwResources.m_transformBuffer.buffer.Get());
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
				BlitzenDX12::CreateResourcesTransitionBarrier(dispatchResetBarrier, rwResources.m_clusterGroupCounter.buffer.Get(), 
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				commandList->ResourceBarrier(1, &dispatchResetBarrier);

				// Descriptors
				commandList->SetComputeRootDescriptorTable(pipelineContext.m_clusterCullTableRootID, descriptorContext.m_cullClusterTableHandle[frame]);

				// Pipeline
				commandList->SetPipelineState(pipelineContext.m_clusterCullCmdResetPso.Get());

				// DISPATCH COUNTER RESET
				commandList->Dispatch(1, 1, 1);

				// Blocks dispatch, waits for cluster dispatch counter reset and dispatch cmd read
				D3D12_RESOURCE_BARRIER clusterDispatchBarriers[2]{};
				// dispatch counter reset
				BlitzenDX12::CreateResourceUAVBarrier(clusterDispatchBarriers[0], rwResources.m_clusterGroupCounter.buffer.Get());
				// dispatch group data read
				BlitzenDX12::CreateResourceUAVBarrier(clusterDispatchBarriers[1], rwResources.m_clusterGroupDataBuffer.buffer.Get());
				// execute
				commandList->ResourceBarrier(BLIT_ARRAY_SIZE(clusterDispatchBarriers), clusterDispatchBarriers);

				commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_CULL_ROOT_HI_Z_MAP_ID, descriptorContext.m_HI_Z_MAP_cullHandle[frame]);

				// Pipelines and constants
				commandList->SetPipelineState(pipelineContext.m_clusterCullDispatchPso.Get());
				commandList->SetComputeRoot32BitConstant(pipelineContext.m_clusterCullWorkRootID, cullContext.m_workCount, 0);

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
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterCullBarriers[1], rwResources.m_clusterGroupCounter.buffer.Get(), 
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				// draw cmd count reset
				BlitzenDX12::CreateResourceUAVBarrier(clusterCullBarriers[2], rwResources.m_clusterVisibilityBuffer.buffer.Get());
				// execute
				commandList->ResourceBarrier(BLIT_ARRAY_SIZE(clusterCullBarriers), clusterCullBarriers);

				commandList->SetPipelineState(pipelineContext.m_clusterCullPso.Get());

				// CULL CLUSTERS
				commandList->ExecuteIndirect(pipelineContext.m_clusterCullCmdSign.Get(), 1, rwResources.m_clusterGroupCounter.buffer.Get(), 0, nullptr, 0);

				/*
					CLUSTER BATCHING PROCESS
				*/
				// BARRIER ON COUNTER BEFORE RESET
				D3D12_RESOURCE_BARRIER resetBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(resetBarrier, rwResources.m_clusterDrawCounter.buffer.Get(),
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				commandList->ResourceBarrier(1, &resetBarrier);

				commandList->SetComputeRootDescriptorTable(pipelineContext.m_clusterCullTableRootID, descriptorContext.m_cullClusterTableHandle[frame]);

				commandList->SetPipelineState(pipelineContext.m_opaqueDynamicCountResetPso.Get());

				// RESET 
				commandList->Dispatch(1, 1, 1);

				D3D12_RESOURCE_BARRIER clusterBatchCmdBarrier{};
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterBatchCmdBarrier, rwResources.m_clusterGroupCounter.buffer.Get(), 
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				commandList->ResourceBarrier(1, &clusterBatchCmdBarrier);

				// NEW COMMAND
				commandList->SetPipelineState(pipelineContext.m_clusterCullBatchCmdPso.Get());
				commandList->Dispatch(1, 1, 1);

				// BAR CLUSTER AND DRAW RESOURCES
				D3D12_RESOURCE_BARRIER clusterBatchBarriers[5]{};
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterBatchBarriers[0], rwResources.m_clusterDrawCounter.buffer.Get(), 
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				BlitzenDX12::CreateResourceUAVBarrier(clusterBatchBarriers[1], rwResources.m_clusterDrawCmdBuffer.buffer.Get());
				BlitzenDX12::CreateResourcesTransitionBarrier(clusterBatchBarriers[2], rwResources.m_clusterGroupCounter.buffer.Get(), 
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				BlitzenDX12::CreateResourceUAVBarrier(clusterBatchBarriers[3], rwResources.m_clusterVisibilityBuffer.buffer.Get());
				BlitzenDX12::CreateResourceUAVBarrier(clusterBatchBarriers[4], rwResources.m_clusterGroupDataBuffer.buffer.Get());
				commandList->ResourceBarrier(BLIT_ARRAY_SIZE(clusterBatchBarriers), clusterBatchBarriers);

				commandList->SetPipelineState(pipelineContext.m_clusterCullBatchPso.Get());
				commandList->ExecuteIndirect(pipelineContext.m_clusterCullCmdSign.Get(), 1, rwResources.m_clusterGroupCounter.buffer.Get(), 0, nullptr, 0);

				// Block graphics, should wait for command and count write
				D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
				// command write
				BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_clusterDrawCmdBuffer.buffer.Get(), 
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
				// count write
				BlitzenDX12::CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_clusterDrawCounter.buffer.Get(), 
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
		auto commandList{ pRenderer->m_cmdContext[frame].m_computeCmdList.Get() };
		auto depthTarget{ pRenderer->m_depthBuffers[swapchainId].Get() };
		auto& rwResources{ pRenderer->m_rwResources[frame] };
		auto& pipelineContext{ pRenderer->m_pipelineContext };
		auto& descriptorContext{ pRenderer->m_descriptorContext };

		// Binds heap for compute
		ID3D12DescriptorHeap* heaps[] = { descriptorContext.m_viewHeap.Get() };
		commandList->SetDescriptorHeaps(1, heaps);

		D3D12_RESOURCE_BARRIER depthPyramidBarriers[1]{};
		BlitzenDX12::CreateResourcesTransitionBarrier(depthPyramidBarriers[0], rwResources.m_HI_Z.pyramid.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(depthPyramidBarriers), depthPyramidBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_HI_Z_MapRoot.Get());
		commandList->SetPipelineState(pipelineContext.m_HI_Z_MapPso.Get());

		UINT mipLevel{ 0 };
		for (uint32_t i = 0; i < rwResources.m_HI_Z.mipCount; ++i)
		{
			// Mip size calculcations
			uint32_t levelWidth = BlitML::Max(1u, (rwResources.m_HI_Z.width) >> i);
			uint32_t levelHeight = BlitML::Max(1u, (rwResources.m_HI_Z.height) >> i);

			// Binds write texture (the depth pyramid has a copy for double buffering and each one has the correct offsets for the descriptor heap)
			commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_HI_Z_MAP_OUTPUT_ID, rwResources.m_HI_Z.mips[i]);

			// Binds read texture (For first level, it's the depth target. For every other level, it's the depth pyramid itself)
			if (i == 0)
			{
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_HI_Z_MAP_INPUT_ID, descriptorContext.m_depthTargetHandle[swapchainId]);
				UINT HI_Z_constants[BlitzenDX12::CE_HI_Z_MAP_CONSTANT_32BIT_COUNT]{ mipLevel, pRenderer->m_swapchainWidth, pRenderer->m_swapchainHeight, levelWidth, levelHeight };
				commandList->SetComputeRoot32BitConstants(BlitzenDX12::CE_HI_Z_MAP_CONSTANT_ID, BlitzenDX12::CE_HI_Z_MAP_CONSTANT_32BIT_COUNT, HI_Z_constants, 0);
			}
			else
			{
				commandList->SetComputeRootDescriptorTable(BlitzenDX12::CE_HI_Z_MAP_INPUT_ID, descriptorContext.m_HI_Z_MAP_cullHandle[frame]);
				UINT HI_Z_constants[BlitzenDX12::CE_HI_Z_MAP_CONSTANT_32BIT_COUNT]{ mipLevel, 0, 0, levelWidth, levelHeight };
				commandList->SetComputeRoot32BitConstants(BlitzenDX12::CE_HI_Z_MAP_CONSTANT_ID, BlitzenDX12::CE_HI_Z_MAP_CONSTANT_32BIT_COUNT, HI_Z_constants, 0);
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
	}
}

#endif