#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vulkanCommands.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanPipelines.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanRNDResources.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vkRuntimeHelpers.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
    void RenderObjects(BlitzenVulkan::VulkanRenderer* pRenderer, const RENDER_CONTEXT& renderContext)
    {
		uint32_t frame{ pRenderer->m_currentFrame };
		auto& readWrites{ pRenderer->m_readWrites[frame] };
        auto& readOnlies{ pRenderer->m_readOnlies };
		auto& descriptorContext{ pRenderer->m_descriptorContext };
		auto& pipelineContext{ pRenderer->m_pipelines };
		auto& cmd{ pRenderer->m_commandsContext[frame] };
		auto cmdb{ cmd.m_mainGraphicsCmdB };
		auto instance{ pRenderer->m_instance };

		switch (renderContext.m_renderType)
		{
		case BLIT_RENDER_TYPE::RENDER_OPAQUE:
		{
			BlitzenVulkan::BeginRendering(cmdb, VkExtent2D{pRenderer->m_drawWidth, pRenderer->m_drawHeight}, { 0, 0 }, 1, &pipelineContext.m_colorTargetInfo[frame], 
				&pipelineContext.m_depthTargetInfo[frame], nullptr);

            pipelineContext.m_colorTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            pipelineContext.m_depthTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

            // Descriptors
            BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, BlitzenVulkan::Ce_PushDescriptorSetID, 
                BlitzenVulkan::Ce_GraphicsDescriptorCount, descriptorContext.m_pushDescriptorsGraphics);
            BlitzenVulkan::PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle,
                BlitzenVulkan::Ce_PushDescriptorSetID, BlitzenVulkan::Ce_SharedDescriptorCount,
                &descriptorContext.m_pushDescriptorsShared[frame * BlitzenVulkan::Ce_SharedDescriptorCount]);
            vkCmdBindDescriptorSets(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, BlitzenVulkan::Ce_TextureDescriptorsSetID, 1,
                &descriptorContext.m_textureDescriptorSet, 0, nullptr);

            // Draw
            vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawPso.handle);
            vkCmdBindIndexBuffer(cmdb, readOnlies.m_idxBuffer.m_buffer.m_handle, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexedIndirectCount(cmdb, readWrites.m_drawCmdBuffer.m_buffer.m_handle, offsetof(BlitzenVulkan::IndirectDrawData, drawIndirect),
                readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, BlitzenVulkan::Ce_DrawCmdElementCount, sizeof(BlitzenVulkan::IndirectDrawData));

			vkCmdEndRendering(cmd.m_mainGraphicsCmdB);

			break;
		}
		default:
		{
			break;
		}
		}
    }

	void FinalizeRendering(BlitzenVulkan::VulkanRenderer* pRenderer)
	{
        BLIT_ASSERT_MESSAGE(pRenderer->m_secondWaitSemaphore != VK_NULL_HANDLE, "The second wait semaphore needs to be passed after culling");

        auto& cmd{ pRenderer->m_commandsContext[pRenderer->m_currentFrame] };

        // COPIES COLOR TARGET TO SWAPCHAIN
        if constexpr (BlitzenCore::Ce_DepthPyramidDebug)
        {
            //BlitzenVulkan::CopyPyramidToSwapchain(cmd.m_mainGraphicsCmdB, m_instance, pRenderer->m_pipelines, m_readOnlies, pRenderer->m_readWrites[pRenderer->m_currentFrame], 
            //    m_descriptorContext, context, pRenderer->m_currentFrame,
            //    pRenderer->m_swapchain, pRenderer->m_swapchainIDX, pRenderer->m_drawWidth, pRenderer->m_drawHeight, context.m_camera.transformData.debugPyramidLevel);
        }
        else
        {
            pRenderer->CopyTargetToSwapchain(cmd.m_mainGraphicsCmdB);
        }

        // SUBMIT
        VkSemaphoreSubmitInfo waitSemaphores[2]{};
        BlitzenVulkan::CreateSemahoreSubmitInfo(waitSemaphores[0], cmd.m_swapchainSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        BlitzenVulkan::CreateSemahoreSubmitInfo(waitSemaphores[1], pRenderer->m_secondWaitSemaphore, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

        VkSemaphoreSubmitInfo signalSemaphore{};
        BlitzenVulkan::CreateSemahoreSubmitInfo(signalSemaphore, cmd.m_renderSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

        BlitzenVulkan::SubmitCommandBuffer(pRenderer->m_graphicsQueue.handle, cmd.m_mainGraphicsCmdB, 2, waitSemaphores, 1, &signalSemaphore, cmd.m_frameFence.handle);
	}

    void SetupForFirstRenderPass(BlitzenVulkan::VulkanRenderer* pRenderer)
    {
        uint32_t frame{ pRenderer->m_currentFrame };
        auto& readWrites{ pRenderer->m_readWrites[frame] };
        auto& cmd{ pRenderer->m_commandsContext[frame] };
        auto& pipelineContext{ pRenderer->m_pipelines };

        BlitzenVulkan::DefineViewportAndScissor(cmd.m_mainGraphicsCmdB, pRenderer->m_swapchain.m_extent);

        BlitzenVulkan::FirstRenderPassBarriers(cmd.m_mainGraphicsCmdB, readWrites.m_colorTarget.m_image.m_image.m_handle, readWrites.m_depthTarget.m_image.m_image.m_handle);

        pipelineContext.m_colorTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        pipelineContext.m_depthTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    }
}