#include "Renderer/Interface/blitRenderer.h"
#include "vkCull.h"
#include "Core/DbLog/blitAssert.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanRNDResources.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vulkanCommands.h"

namespace BlitzenEngine
{
	void DispatchCullingShaders(BlitzenVulkan::VulkanRenderer* pRenderer, const CULL_CONTEXT& cullContext)
	{
		uint32_t frame{ pRenderer->m_currentFrame };
		auto& readWrites{ pRenderer->m_readWrites[frame] };
		auto& descriptorContext{ pRenderer->m_descriptorContext };
		auto& pipelineContext{ pRenderer->m_pipelines };
		auto& cmd{ pRenderer->m_commandsContext[frame] };
		auto cmdb{ cmd.m_mainGraphicsCmdB };
		auto instance{ pRenderer->m_instance };

		switch (cullContext.m_cullType)
		{
		case BLIT_CULL_TYPE::DRAW_CULL_DEFAULT:
		{
			switch (cullContext.m_workType)
			{
			case RENDER_OBJECT_TYPE::OPAQUE_STATIC:
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
		case BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION:
		{
			switch (cullContext.m_workType)
			{
			case RENDER_OBJECT_TYPE::OPAQUE_STATIC:
			{
				// Count reset barrier
				VkBufferMemoryBarrier2 resetBarrier{};
				BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, resetBarrier, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
					VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
				// Execute 
				BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, 1, &resetBarrier, 0, nullptr);

				// Reset
				vkCmdFillBuffer(cmdb, readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

				// Barrier waits for count reset, last frame draw commands read and visibility buffer write
				VkBufferMemoryBarrier2 cullingBarriers[2]{};
				// Count reset barrier
				BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, cullingBarriers[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
				// Commands read barrier
				BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, cullingBarriers[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
					VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);

				// Additional image memory barrier for depth pyramid
				VkImageMemoryBarrier2 HI_Z_barrier{};
				BlitzenVulkan::ImageMemoryBarrier(readWrites.m_HI_Z_MAP.m_pyramid.m_image.m_handle, HI_Z_barrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
					VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

				// execute
				BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers, 1, &HI_Z_barrier);

				descriptorContext.m_HI_Z_descInfo[frame].imageView = readWrites.m_HI_Z_MAP.m_pyramid.m_view.m_handle;

				// Descriptors
				BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, BlitzenVulkan::Ce_PushDescriptorSetID, 
					BlitzenVulkan::Ce_CullDescriptorCount, &descriptorContext.m_pushDescriptorsCull[BlitzenVulkan::Ce_CullDescriptorCount * frame]);
				BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, BlitzenVulkan::Ce_PushDescriptorSetID, 
					BlitzenVulkan::Ce_SharedDescriptorCount, &descriptorContext.m_pushDescriptorsShared[BlitzenVulkan::Ce_SharedDescriptorCount * frame]);
				BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, BlitzenVulkan::Ce_PushDescriptorSetID, 1,
					&descriptorContext.m_HI_Z_cullDescriptor[frame]);

				// Pipeline and descriptors
				vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawTemporalOccPso.handle);
				vkCmdPushConstants(cmdb, pipelineContext.m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &cullContext.m_workCount);

				// Dispatch
				vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(cullContext.m_workCount, 64), 1, 1);

				// Barrier blocks graphics command and count read
				VkBufferMemoryBarrier2 graphicsBarrier[2]{};
				// Count
				BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, graphicsBarrier[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
				// Commands
				BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, graphicsBarrier[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
					VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
				// Execute
				BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(graphicsBarrier), graphicsBarrier, 0, nullptr);

				pRenderer->m_secondWaitSemaphore = cmd.m_bufferUpdateSemaphore.handle;
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
		case BLIT_CULL_TYPE::CLUSTER_CULL_DEFAULT:
		{
			auto device{ pRenderer->m_device };

			// Fist culling pass with separate command buffer
			BlitzenVulkan::BeginCommandBuffer(cmd.m_computeCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			// Barrier before count reset
			VkBufferMemoryBarrier2 clusterResetBarrier{};
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, clusterResetBarrier, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
			// execute
			BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, 1, &clusterResetBarrier, 0, nullptr);

			// Reset
			vkCmdFillBuffer(cmdb, readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

			// Barrier for previous frame cluster count and cluster dispatch read
			VkBufferMemoryBarrier2 cullBarriers[2]{};
			// Cluster count
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, cullBarriers[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
			// Cluster dispatch
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_clusterGroupDataBuffer.m_buffer.m_handle, cullBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);

			// Execute
			BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullBarriers), cullBarriers, 0, nullptr);

			// Descriptors
			BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, BlitzenVulkan::Ce_PushDescriptorSetID, 
				BlitzenVulkan::Ce_CullDescriptorCount, &descriptorContext.m_pushDescriptorsCull[BlitzenVulkan::Ce_CullDescriptorCount * frame]);
			BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, BlitzenVulkan::Ce_PushDescriptorSetID, 
				BlitzenVulkan::Ce_SharedDescriptorCount, &descriptorContext.m_pushDescriptorsShared[BlitzenVulkan::Ce_SharedDescriptorCount * frame]);

			// Pipeline and push constants
			vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullDispatchPso.handle);
			vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &cullContext.m_workCount);
			// Dispatch
			vkCmdDispatch(cmdb, (cullContext.m_workCount / 64) + 1, 1, 1);

			// Cluster dispatch read barrier
			VkBufferMemoryBarrier2 clusterCullBarrier{};
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_clusterGroupDataBuffer.m_buffer.m_handle, clusterCullBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
			// execute
			BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, 1, &clusterCullBarrier, 0, nullptr);

			// Cluster count copy barrier
			VkBufferMemoryBarrier2 countCopyBarrier{};
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, countCopyBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, 0, sizeof(uint32_t));
			BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, 1, &countCopyBarrier, 0, nullptr);
			// Copy
			BlitzenVulkan::CopyBufferToBuffer(cmdb, readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, readWrites.m_clusterDispatchCounterCopy.m_buffer.m_handle, sizeof(uint32_t), 0, 0);

			// Submits command buffer to generate cluster dispatch count
			VkSemaphoreSubmitInfo bufferUpdateWaitSemaphore{};
			BlitzenVulkan::CreateSemahoreSubmitInfo(bufferUpdateWaitSemaphore, cmd.m_bufferUpdateSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

			VkSemaphoreSubmitInfo waitForClusterData{};
			BlitzenVulkan::CreateSemahoreSubmitInfo(waitForClusterData, cmd.m_clusterSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
			BlitzenVulkan::SubmitCommandBuffer(pRenderer->m_computeQueue.handle, cmd.m_computeCmdB, 1, &bufferUpdateWaitSemaphore, 1, &waitForClusterData, cmd.m_preClusterFence.handle);

			// FENCE DISPATCH
			vkWaitForFences(device, 1, &cmd.m_preClusterFence.handle, VK_TRUE, BlitzenVulkan::ce_fenceTimeout);
			vkResetFences(device, 1, &cmd.m_preClusterFence.handle);

			uint32_t dispatchCount{ uint32_t(*reinterpret_cast<uint32_t*>(readWrites.m_clusterDispatchCounterCopy.m_buffer.m_vmaInfo.pMappedData)) };

			// Draw count reset barrier
			VkBufferMemoryBarrier2 drawCountResetBarrier{};
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, drawCountResetBarrier, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
				VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, sizeof(uint32_t));
			BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, 1, &drawCountResetBarrier, 0, nullptr);

			vkCmdFillBuffer(cmdb, readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

			// Wait for draw count reset, previous frame command read and cluster dispatch write
			VkBufferMemoryBarrier2 cullingShaders[3] = {};
			// Draw count reset
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, cullingShaders[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
			// Command read barrier
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, cullingShaders[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
				VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
			// Cluster dispatch barrier
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_clusterGroupDataBuffer.m_buffer.m_handle, cullingShaders[2], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
			// Execution
			BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullingShaders), cullingShaders, 0, nullptr);

			// Descriptors
			BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, BlitzenVulkan::Ce_PushDescriptorSetID, 
				BlitzenVulkan::Ce_CullDescriptorCount, &descriptorContext.m_pushDescriptorsCull[frame * BlitzenVulkan::Ce_CullDescriptorCount]);
			BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, BlitzenVulkan::Ce_PushDescriptorSetID, 
				BlitzenVulkan::Ce_SharedDescriptorCount, &descriptorContext.m_pushDescriptorsShared[frame * BlitzenVulkan::Ce_SharedDescriptorCount]);
			BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, BlitzenVulkan::Ce_PushDescriptorSetID, 
				BlitzenVulkan::Ce_ClusterCullDescriptorCount, descriptorContext.m_pushDescriptorsClusterCull);

			// Pipeline and push constants
			vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullPso.handle);
			vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &dispatchCount);
			// Dispatch
			vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(dispatchCount, 64), 1, 1);

			// Barriers stop graphics command read and count read
			VkBufferMemoryBarrier2 waitForCullingShader[2]{};
			// Count read
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, waitForCullingShader[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
			// Command read
			BlitzenVulkan::BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, waitForCullingShader[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
				0, VK_WHOLE_SIZE);
			// Execute
			BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(waitForCullingShader), waitForCullingShader, 0, nullptr);

			pRenderer->m_secondWaitSemaphore = cmd.m_clusterSemaphore.handle;
			break;
		}
		default:
		{
			BLIT_ASSERT_MESSAGE(true, "Requested suspended culling type");
		}
		}
	}

	void GenerateHI_Z_MAP(BlitzenVulkan::VulkanRenderer* pRenderer)
	{
		uint32_t frame{ pRenderer->m_currentFrame };
		auto& readWrites{ pRenderer->m_readWrites[frame] };
		auto& descriptorContext{ pRenderer->m_descriptorContext };
		auto& pipelineContext{ pRenderer->m_pipelines };
		auto& cmd{ pRenderer->m_commandsContext[frame] };
		auto cmdb{ cmd.m_mainGraphicsCmdB };
		auto instance{ pRenderer->m_instance };

		VkImageMemoryBarrier2 HI_Z_barriers[2]{};
		// Depth attachment to shader read
		BlitzenVulkan::ImageMemoryBarrier(readWrites.m_depthTarget.m_image.m_image.m_handle, HI_Z_barriers[0], VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
		// Depth pyramid to shader write
		BlitzenVulkan::ImageMemoryBarrier(readWrites.m_HI_Z_MAP.m_pyramid.m_image.m_handle, HI_Z_barriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
		// Execute
		BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 2, HI_Z_barriers);

		// Creates the descriptor write array. Initially it will holds the depth attachment layout and image view
		descriptorContext.m_depthTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorContext.m_depthTargetDescInfo[frame].imageView = readWrites.m_depthTarget.m_image.m_view.m_handle;
		descriptorContext.m_depthTargetDescInfo[frame].sampler = readWrites.m_depthTarget.m_samp.m_handle;

		// Binds the compute pipeline. It will be dispatched for every loop iteration
		vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_hiZPso.handle);
		for (size_t i = 0; i < readWrites.m_HI_Z_MAP.m_levelCount; ++i)
		{
			// Updates image info for each iteration
			if (i != 0)
			{
				descriptorContext.m_depthTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				descriptorContext.m_depthTargetDescInfo[frame].imageView = readWrites.m_HI_Z_MAP.m_levels[i - 1];
			}

			descriptorContext.m_HI_Z_descInfo[frame].imageView = readWrites.m_HI_Z_MAP.m_levels[i];

			// Descriptors
			BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_hiZLayout.handle, 0, 2, &descriptorContext.m_HI_Z_descriptors[2 * frame]);

			// Mip size calculcations
			uint32_t levelWidth = BlitML::Max(1u, (readWrites.m_HI_Z_MAP.m_pyramid.m_width) >> i);
			uint32_t levelHeight = BlitML::Max(1u, (readWrites.m_HI_Z_MAP.m_pyramid.m_height) >> i);

			// Push constant for extent
			BlitML::vec2 pyramidLevelExtentPushConstant{ float(levelWidth), float(levelHeight) };
			vkCmdPushConstants(cmdb, pipelineContext.m_hiZLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlitML::vec2), &pyramidLevelExtentPushConstant);

			// Dispatch the shader to generate the current mip level of the depth pyramid
			vkCmdDispatch(cmdb, levelWidth / 32 + 1, levelHeight / 32 + 1, 1);

			// Barrier for the next loop, since it will use the current mip as the read descriptor
			VkImageMemoryBarrier2 hizWriteBarrier{};
			BlitzenVulkan::ImageMemoryBarrier(readWrites.m_HI_Z_MAP.m_pyramid.m_image.m_handle, hizWriteBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
			BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 1, &hizWriteBarrier);
		}

		// Pipeline barrier to transition back to depth attachment optimal layout
		VkImageMemoryBarrier2 depthAttachmentReadBarrier{};
		BlitzenVulkan::ImageMemoryBarrier(readWrites.m_depthTarget.m_image.m_image.m_handle, depthAttachmentReadBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
		BlitzenVulkan::PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 1, &depthAttachmentReadBarrier);
	}
}