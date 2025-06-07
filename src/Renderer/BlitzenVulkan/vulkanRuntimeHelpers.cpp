#include "vulkanRenderer.h"
#include "vulkanResourceFunctions.h"
#include "vulkanRNDResources.h"
#include "vulkanPipelines.h"

namespace BlitzenVulkan
{
    void VulkanRenderer::UpdateObjectTransform(uint32_t transformId, BlitzenEngine::MeshTransform* pTransform)
    {
        auto pData = m_readWrites[m_currentFrame].m_transformBuffer.m_pMapped;
        BlitzenCore::BlitMemCopy(pData + transformId, pTransform, sizeof(BlitzenEngine::MeshTransform));
    }

    void VulkanRenderer::CopyTargetToSwapchain(VkCommandBuffer cmdb)
    {
        auto& readWrites{ m_readWrites[m_currentFrame] };
        auto& cmd{ m_commandsContext[m_currentFrame] };

        // Image barriers to transition the layout of the color attachment and the swapchain image
        VkImageMemoryBarrier2 presentBarriers[2]{};
        ImageMemoryBarrier(readWrites.m_colorTarget.m_image.m_image.m_handle, presentBarriers[0], VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        ImageMemoryBarrier(m_swapchain.m_images[m_swapchainIDX], presentBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, BLIT_ARRAY_SIZE(presentBarriers), presentBarriers);

        VkDescriptorImageInfo swapchainDescInfo{};
        swapchainDescInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        swapchainDescInfo.imageView = m_swapchain.m_views[m_swapchainIDX];

        VkWriteDescriptorSet swapchainImageWrite{};
        WriteImageDescriptorSets(swapchainImageWrite, &swapchainDescInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, 1, Ce_SwapchainDescriptorBinding);

        VkWriteDescriptorSet colorAttachmentCopyWrite[2]{ m_descriptorContext.m_colorTargetDescriptor[m_currentFrame], swapchainImageWrite };
        PushDescriptors(m_instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.m_presentLayout.handle, 0, 2, colorAttachmentCopyWrite);

        // Extent push constant
        BlitML::vec2 presentImageExtentPcVal{ float(readWrites.m_colorTarget.m_image.m_width), float(readWrites.m_colorTarget.m_image.m_height) };
        vkCmdPushConstants(cmdb, m_pipelines.m_presentLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlitML::vec2), &presentImageExtentPcVal);

        // Dispatches copy shader
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.m_presentPso.handle);
        vkCmdDispatch(cmdb, readWrites.m_colorTarget.m_image.m_width / 8 + 1, readWrites.m_colorTarget.m_image.m_height / 8 + 1, 1);

        // Layout transition barrier
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(m_swapchain.m_images[m_swapchainIDX], presentImageBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);
    }

    void VulkanRenderer::Present(uint8_t isLoading)
    {
        auto& cmd = m_commandsContext[m_currentFrame];

        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.pNext = nullptr;

        info.swapchainCount = 1;
        info.pSwapchains = &m_swapchain.m_handle;

#if defined(DASHER_JOIN)

        VkSemaphore waitSemaphores[2]{ cmd.m_renderSemaphore.handle, cmd.m_dasherRenderSemaphore.handle };
        if (isLoading)
        {
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = waitSemaphores;
        }
        else
        {
            info.waitSemaphoreCount = BLIT_ARRAY_SIZE(waitSemaphores);
            info.pWaitSemaphores = waitSemaphores;
        }
#else

        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &cmd.m_renderSemaphore.handle;
#endif

        info.pImageIndices = &m_swapchainIDX;
        info.pResults = nullptr;

        vkQueuePresentKHR(m_graphicsQueue.handle, &info);

        m_currentFrame = (m_currentFrame + 1) % ce_framesInFlight;
    }

    static void RecreateSwapchain(VkDevice device, VkInstance instance, Swapchain& swapchainData, VkSurfaceKHR surface, VkPhysicalDevice pdv, VmaAllocator vma,
        PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources* readWrites, DescriptorContext& descriptorContext, uint32_t windowWidth, uint32_t windowHeight, uint32_t frame, Queue graphicsQueue, Queue presentQueue, Queue computeQueue)
    {
        vkDeviceWaitIdle(device);

        for (uint32_t img = 0; img < swapchainData.m_imageCount; ++img)
        {
            vkDestroyImageView(device, swapchainData.m_views[img], nullptr);
        }
        swapchainData.m_imageCount = 0;

        // Creates new swapchain, after saving the old handle to destroy it
        auto oldSwapchain = swapchainData.m_handle;
        CreateSwapchain(device, surface, pdv, windowWidth, windowHeight, graphicsQueue, presentQueue, computeQueue, nullptr, swapchainData, oldSwapchain);

        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);

        for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            auto& rws{ readWrites[frame] };

            // Destroys old color target
            vmaDestroyImage(vma, rws.m_colorTarget.m_image.m_image.m_handle, rws.m_colorTarget.m_image.m_image.m_vmaAlloc);
            vkDestroyImageView(device, rws.m_colorTarget.m_image.m_view.m_handle, nullptr);

            BLIT_ASSERT(Create2DImageResource(device, vma, rws.m_colorTarget.m_image, windowWidth, windowHeight, Ce_ColorTargetFormat, Ce_ColorTargetUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY));

            descriptorContext.m_colorTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorContext.m_colorTargetDescInfo[frame].imageView = rws.m_colorTarget.m_image.m_view.m_handle;
            descriptorContext.m_colorTargetDescInfo[frame].sampler = rws.m_colorTarget.m_samp.m_handle;

            WriteImageDescriptorSets(descriptorContext.m_colorTargetDescriptor[frame], &descriptorContext.m_colorTargetDescInfo[frame], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE,
                Ce_ColorTargetDescriptorBinding);

            // Color attachment rendering info
            CreateRenderingAttachmentInfo(pipelineContext.m_colorTargetInfo[frame], rws.m_colorTarget.m_image.m_view.m_handle, Ce_ColorTargetLayout,
                VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, ce_WindowClearColor);

            // Destroys old depth target
            vmaDestroyImage(vma, rws.m_depthTarget.m_image.m_image.m_handle, rws.m_depthTarget.m_image.m_image.m_vmaAlloc);
            vkDestroyImageView(device, rws.m_depthTarget.m_image.m_view.m_handle, nullptr);

            BLIT_ASSERT(Create2DImageResource(device, vma, rws.m_depthTarget.m_image, windowWidth, windowHeight, Ce_DepthTargetFormat, Ce_DepthTargetUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY));

            descriptorContext.m_depthTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorContext.m_depthTargetDescInfo[frame].imageView = rws.m_depthTarget.m_image.m_view.m_handle;
            descriptorContext.m_depthTargetDescInfo[frame].sampler = rws.m_depthTarget.m_samp.m_handle;

            WriteImageDescriptorSets(descriptorContext.m_HI_Z_descriptors[Ce_DepthTargetDescriptorID + frame * 2], &descriptorContext.m_depthTargetDescInfo[frame],
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE, Ce_DepthTargetDescriptorBinding);

            CreateRenderingAttachmentInfo(pipelineContext.m_depthTargetInfo[frame], rws.m_depthTarget.m_image.m_view.m_handle, Ce_DepthTargetLayout, VK_ATTACHMENT_LOAD_OP_LOAD,
                VK_ATTACHMENT_STORE_OP_STORE, { 0, 0, 0, 0 }, { 0, 0 });

            // Destroys old depth pyramid
            for (uint32_t level = 0; level < rws.m_HI_Z_MAP.m_levelCount; ++level)
            {
                vkDestroyImageView(device, rws.m_HI_Z_MAP.m_levels[level], nullptr);
            }
            vmaDestroyImage(vma, rws.m_HI_Z_MAP.m_pyramid.m_image.m_handle, rws.m_HI_Z_MAP.m_pyramid.m_image.m_vmaAlloc);
            vkDestroyImageView(device, rws.m_HI_Z_MAP.m_pyramid.m_view.m_handle, nullptr);

            BLIT_ASSERT(CreateHI_Z(device, vma, descriptorContext, rws.m_HI_Z_MAP, rws.m_colorTarget.m_image.m_width, rws.m_colorTarget.m_image.m_height, frame, rws.m_depthTarget.m_samp.m_handle));
        }
    }

    BlitML::vec2 VulkanRenderer::UpdateWindow(uint32_t windowWidth, uint32_t windowHeight, void* pHandle)
    {
        m_drawWidth = windowWidth;
        m_drawHeight = windowHeight;

        RecreateSwapchain(m_device, m_instance, m_swapchain, m_surface.handle, m_physicalDevice, m_allocator, m_pipelines, m_readOnlies, m_readWrites, m_descriptorContext,
            m_drawWidth, m_drawHeight, m_currentFrame, m_graphicsQueue, m_presentQueue, m_computeQueue);

        return BlitML::vec2{ float(m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_width), float(m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_height) };
    }
}