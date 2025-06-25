#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"
#include "vkRuntimeHelpers.h"
#include "vulkanCommands.h" 
#include "BlitCL/blitDynamicArr.h"

namespace BlitzenEngine
{
    void PresentRender(BlitzenVulkan::VulkanRenderer* pRenderer, uint32_t waitCount)
    {
        auto& cmd = pRenderer->m_commandsContext[pRenderer->m_currentFrame];

        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.pNext = nullptr;

        info.swapchainCount = 1;
        info.pSwapchains = &pRenderer->m_swapchain.m_handle;

#if defined(DASHER_JOIN)

        VkSemaphore waitSemaphores[2]{ cmd.m_renderSemaphore.handle, cmd.m_dasherRenderSemaphore.handle };
        info.waitSemaphoreCount = waitCount;
        info.pWaitSemaphores = waitSemaphores;

#else

        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &cmd.m_renderSemaphore.handle;
#endif

        info.pImageIndices = &pRenderer->m_swapchainIDX;
        info.pResults = nullptr;

        vkQueuePresentKHR(pRenderer->m_graphicsQueue.handle, &info);

        pRenderer->m_currentFrame = (pRenderer->m_currentFrame + 1) % BlitzenVulkan::ce_framesInFlight;
    }

    void PlaceRendererFence(BlitzenVulkan::VulkanRenderer* pRenderer, RENDERER_FENCE_TYPE fenceType)
    {
        auto& cmd{ pRenderer->m_commandsContext[pRenderer->m_currentFrame] };

        vkWaitForFences(pRenderer->m_device, 1, &cmd.m_frameFence.handle, VK_TRUE, BlitzenVulkan::ce_fenceTimeout);
        VK_CHECK_MSG(vkResetFences(pRenderer->m_device, 1, &cmd.m_frameFence.handle));
    }

    void UpdateRendererView(BlitzenVulkan::VulkanRenderer* pRenderer, CameraViewData& viewData, bool isFrustumFrozen)
    {
        auto& cmd{ pRenderer->m_commandsContext[pRenderer->m_currentFrame] };

        if (isFrustumFrozen)
        {
            // Only change the matrix that moves the camera if the freeze frustum debug functionality is active
            pRenderer->m_readWrites[pRenderer->m_currentFrame].m_viewDataBuffer.m_pMapped->projectionViewMatrix = viewData.projectionViewMatrix;
        }
        else
        {
            BlitzenCore::BlitMemCopy(pRenderer->m_readWrites[pRenderer->m_currentFrame].m_viewDataBuffer.m_pMapped, &viewData, sizeof(viewData));
        }

        // Swapchain image
        vkAcquireNextImageKHR(pRenderer->m_device, pRenderer->m_swapchain.m_handle, BlitzenVulkan::ce_swapchainImageTimeout, cmd.m_swapchainSemaphore.handle, VK_NULL_HANDLE, &pRenderer->m_swapchainIDX);

        BlitzenVulkan::BeginCommandBuffer(cmd.m_mainGraphicsCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    }

    void UpdateRendererTransforms(BlitzenVulkan::VulkanRenderer* pRenderer)
    {
        auto& cmdContext{ pRenderer->m_commandsContext[pRenderer->m_currentFrame] };
        auto& buffers{ pRenderer->m_readWrites[pRenderer->m_currentFrame] };

        BlitzenVulkan::BeginCommandBuffer(cmdContext.m_transferCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        BlitzenVulkan::CopyBufferToBuffer(cmdContext.m_transferCmdB, buffers.m_transformBuffer.m_staging.m_buffer.m_handle, buffers.m_transformBuffer.m_buffer.m_buffer.m_handle,
            buffers.m_transformBuffer.m_staging.m_dataSize, 0, 0);

        // VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT is used here because the signal comes from a transfer queue.
        // More specific shader stages (like VERTEX or COMPUTE) are invalid for transfer queues per Vulkan spec.
        // This ensures compatibility with graphics queue work that reads the transform buffer.
        // DO NOT WASTE TIME TRYING TO CHANGE THIS
        VkSemaphoreSubmitInfo bufferCopySemaphoreInfo{};
        BlitzenVulkan::CreateSemahoreSubmitInfo(bufferCopySemaphoreInfo, cmdContext.m_bufferUpdateSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
        BlitzenVulkan::SubmitCommandBuffer(pRenderer->m_transferQueue.handle, cmdContext.m_transferCmdB, 0, nullptr, 1, &bufferCopySemaphoreInfo, VK_NULL_HANDLE);
    }

    BlitML::vec2 UpdateRendererWindowData(BlitzenVulkan::VulkanRenderer* pRenderer, uint32_t newWidth, uint32_t newHeight, BlitzenPlatform::PlatformContext* pbpHandle)
    {
        pRenderer->m_drawWidth = newWidth;
        pRenderer->m_drawHeight = newHeight;

        // AT THIS POINT JUST PASS THE RENDERER... whatever, should fix this at some point
        BlitzenVulkan::RecreateSwapchain(pRenderer->m_device, pRenderer->m_instance, pRenderer->m_swapchain, pRenderer->m_surface.handle, pRenderer->m_physicalDevice, pRenderer->m_allocator, 
            pRenderer->m_pipelines, pRenderer->m_readOnlies, pRenderer->m_readWrites, pRenderer->m_descriptorContext,
            pRenderer->m_drawWidth, pRenderer->m_drawHeight, pRenderer->m_currentFrame, pRenderer->m_graphicsQueue, pRenderer->m_presentQueue, pRenderer->m_computeQueue);

        return BlitML::vec2{ float(pRenderer->m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_width), float(pRenderer->m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_height) };
    }

    void PrepareRendererForRuntime(BlitzenVulkan::VulkanRenderer* pRenderer)
    {
        vkDeviceWaitIdle(pRenderer->m_device);

		uint32_t dummyTextureOffset = pRenderer->m_readOnlies.m_textureCount;
        BlitCL::DynamicArray<VkImageMemoryBarrier2> dymmyTextureMemoryBarriers{ BlitzenCore::Ce_MaxTextureCount, VkImageMemoryBarrier2{} };

        for (uint32_t tex = dummyTextureOffset; tex < BlitzenCore::Ce_MaxTextureCount; ++tex)
        {
            BlitzenVulkan::ImageMemoryBarrier(pRenderer->m_readOnlies.m_textures[tex].image.m_image.m_handle, dymmyTextureMemoryBarriers[tex], VK_PIPELINE_STAGE_2_COPY_BIT,
                VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        }

		BlitzenVulkan::BeginCommandBuffer(pRenderer->m_commandsContext[0].m_transferCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        BlitzenVulkan::PipelineBarrier(pRenderer->m_commandsContext[0].m_transferCmdB, 0, nullptr, 0, nullptr, BlitzenCore::Ce_MaxTextureCount - dummyTextureOffset, 
            &dymmyTextureMemoryBarriers[dummyTextureOffset]);

        BlitzenVulkan::SubmitCommandBuffer(pRenderer->m_transferQueue.handle, pRenderer->m_commandsContext[0].m_transferCmdB, 0, nullptr, 0, nullptr, VK_NULL_HANDLE);
        vkQueueWaitIdle(pRenderer->m_transferQueue.handle);
	}
}