#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
    void VulkanRenderer::UploadDataToGPUAndSetupForRendering()
    {
        FrameToolsInit();

        CreateImage(m_device, m_allocator, m_colorAttachment, {m_drawExtent.width, m_drawExtent.height, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        VkShaderModule vertexShaderModule;
        VkPipelineShaderStageCreateInfo shaderStages[2];
        CreateShaderProgram(m_device, "VulkanShaders/MainObjectShader.vert.glsl.spv", VK_SHADER_STAGE_VERTEX_BIT, "main", vertexShaderModule, 
        shaderStages[0], nullptr);
    }

    void VulkanRenderer::FrameToolsInit()
    {
        VkCommandPoolCreateInfo commandPoolsInfo {};
        commandPoolsInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolsInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow each individual command buffer to be reset
        commandPoolsInfo.queueFamilyIndex = m_graphicsQueue.index;

        VkCommandBufferAllocateInfo commandBuffersInfo{};
        commandBuffersInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBuffersInfo.pNext = nullptr;
        commandBuffersInfo.commandBufferCount = 1;
        commandBuffersInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceInfo.pNext = nullptr;

        VkSemaphoreCreateInfo semaphoresInfo{};
        semaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoresInfo.flags = 0;
        semaphoresInfo.pNext = nullptr;

        for(size_t i = 0; i < BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT; ++i)
        {
            FrameTools& frameTools = m_frameToolsList[i];
            VK_CHECK(vkCreateCommandPool(m_device, &commandPoolsInfo, m_pCustomAllocator, &(frameTools.mainCommandPool)));
            commandBuffersInfo.commandPool = frameTools.mainCommandPool;
            VK_CHECK(vkAllocateCommandBuffers(m_device, &commandBuffersInfo, &(frameTools.commandBuffer)));

            VK_CHECK(vkCreateFence(m_device, &fenceInfo, m_pCustomAllocator, &(frameTools.inFlightFence)))
            VK_CHECK(vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator, &(frameTools.imageAcquiredSemaphore)))
            VK_CHECK(vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator, &(frameTools.readyToPresentSemaphore)))
        }
    }



    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        Every operation needed for drawing a single frame is put here
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    void VulkanRenderer::DrawFrame(RenderContext& context)
    {
        if(context.windowResize)
        {
            RecreateSwapchain(context.windowWidth, context.windowHeight);
        }

        // Gets a ref to the frame tools of the current frame, so that it doesn't index into the array every time
        FrameTools& fTools = m_frameToolsList[m_currentFrame];

        // Waits for the fence in the current frame tools struct to be signaled and resets it for next time when it gets signalled
        vkWaitForFences(m_device, 1, &(fTools.inFlightFence), VK_TRUE, 1000000000);
        VK_CHECK(vkResetFences(m_device, 1, &(fTools.inFlightFence)))

        // Asks for the next image in the swapchain to use for presentation, and saves it in swapchainIdx
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_initHandles.swapchain, 1000000000, fTools.imageAcquiredSemaphore, VK_NULL_HANDLE, &swapchainIdx);

        BeginCommandBuffer(fTools.commandBuffer, 0);

        // Pipeline barrier to transtion the layout of the color attachment from undefined to optimal
        {
            VkImageMemoryBarrier2 colorAttachmentBarrier{};
            VkImageSubresourceRange colorAttachmentSubresource{};
            colorAttachmentSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentSubresource.baseMipLevel = 0;
            colorAttachmentSubresource.levelCount = VK_REMAINING_MIP_LEVELS;
            colorAttachmentSubresource.baseArrayLayer = 0;
            colorAttachmentSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentBarrier, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, 
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 
            colorAttachmentSubresource);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &colorAttachmentBarrier);
        }
        // Command for color attachment transition recorded

        // Clearing the color attachment
        {
            // Hard coding the color of the screen for now. At some there will be a skybox and it will be calculated with compute shaders
            VkClearColorValue clearColorAttachment{};
            clearColorAttachment.float32[0] = 0.f;
            clearColorAttachment.float32[1] = 0.f;
            clearColorAttachment.float32[2] = 0.f;
            clearColorAttachment.float32[3] = 0.f;
            VkImageSubresourceRange clearColorSubresourceRange{};
            clearColorSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            clearColorSubresourceRange.baseMipLevel = 0;
            clearColorSubresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            clearColorSubresourceRange.baseArrayLayer = 0;
            clearColorSubresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            vkCmdClearColorImage(fTools.commandBuffer, m_colorAttachment.image, VK_IMAGE_LAYOUT_GENERAL, &clearColorAttachment, 1, &clearColorSubresourceRange);
        }

        // Copying the color attachment to the swapchain image and transitioning the image to present
        {
            // color attachment barrier
            VkImageMemoryBarrier2 colorAttachmentBarrier{};
            VkImageSubresourceRange colorAttachmentSubresource{};
            colorAttachmentSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentSubresource.baseMipLevel = 0;
            colorAttachmentSubresource.levelCount = VK_REMAINING_MIP_LEVELS;
            colorAttachmentSubresource.baseArrayLayer = 0;
            colorAttachmentSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentBarrier, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | 
            VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, 
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, colorAttachmentSubresource);

            //swapchain image barrier
            VkImageMemoryBarrier2 swapchainImageBarrier{};
            VkImageSubresourceRange swapchainImageSR{};
            swapchainImageSR.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            swapchainImageSR.baseMipLevel = 0;
            swapchainImageSR.levelCount = VK_REMAINING_MIP_LEVELS;
            swapchainImageSR.baseArrayLayer = 0;
            swapchainImageSR.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], swapchainImageBarrier, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, 
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, swapchainImageSR);

            VkImageMemoryBarrier2 firstTransferMemoryBarriers[2] = {colorAttachmentBarrier, swapchainImageBarrier};

            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, firstTransferMemoryBarriers);

            // Copy the color attachment to the swapchain image
            VkImageSubresourceLayers colorAttachmentSL{};
            colorAttachmentSL.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentSL.baseArrayLayer = 0;
            colorAttachmentSL.layerCount = 1;
            colorAttachmentSL.mipLevel = 0;
            VkImageSubresourceLayers swapchainImageSL{};
            swapchainImageSL.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            swapchainImageSL.baseArrayLayer = 0;
            swapchainImageSL.layerCount = 1;
            swapchainImageSL.mipLevel = 0;
            CopyImageToImage(fTools.commandBuffer, m_colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
            m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_drawExtent, 
            m_initHandles.swapchainExtent, colorAttachmentSL, swapchainImageSL);

            // Change the swapchain image layout to present
            VkImageMemoryBarrier2 presentImageBarrier{};
            VkImageSubresourceRange presentImageSR{};
            presentImageSR.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            presentImageSR.baseMipLevel = 0;
            presentImageSR.levelCount = VK_REMAINING_MIP_LEVELS;
            presentImageSR.baseArrayLayer = 0;
            presentImageSR.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], presentImageBarrier, VK_PIPELINE_STAGE_2_BLIT_BIT, 
            VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, presentImageSR);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);

        }
        // Swapchain image ready to be presented

        SubmitCommandBuffer(m_graphicsQueue.handle, fTools.commandBuffer, fTools.imageAcquiredSemaphore, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, 
        fTools.readyToPresentSemaphore, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, fTools.inFlightFence);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &fTools.readyToPresentSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &(m_initHandles.swapchain);
        presentInfo.pImageIndices = &swapchainIdx;
        vkQueuePresentKHR(m_presentQueue.handle, &presentInfo);

        m_currentFrame = (m_currentFrame + 1) % BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT;
    }

    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags)
    {
        vkResetCommandBuffer(commandBuffer, 0);
        VkCommandBufferBeginInfo commandBufferInfo{};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferInfo.pNext = nullptr;
        commandBufferInfo.pInheritanceInfo = nullptr;
        commandBufferInfo.flags = usageFlags;
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferInfo));
    }

    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkPipelineStageFlags2 waitPipelineStage,
    VkSemaphore signalSemaphore, VkPipelineStageFlags2 signalPipelineStage, VkFence fence /*= VK_NULL_HANDLE*/)
    {
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSemaphoreSubmitInfo waitSemaphoreInfo{};
        waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        waitSemaphoreInfo.stageMask = waitPipelineStage;
        waitSemaphoreInfo.semaphore = waitSemaphore;

        VkSemaphoreSubmitInfo signalSemaphoreInfo{};
        signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalSemaphoreInfo.stageMask = signalPipelineStage;
        signalSemaphoreInfo.semaphore = signalSemaphore;

        VkCommandBufferSubmitInfo commandBufferInfo{};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferInfo.commandBuffer = commandBuffer;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &commandBufferInfo;
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;
        vkQueueSubmit2(queue, 1, &submitInfo, fence);
    }

    void VulkanRenderer::RecreateSwapchain(uint32_t windowWidth, uint32_t windowHeight)
    {
        //Wait for the current frame to be done with the swapchain
        vkDeviceWaitIdle(m_device);

        //Destroy the current swapchain
        for(size_t i = 0; i < m_initHandles.swapchainImageViews.GetSize(); ++i)
        {
            vkDestroyImageView(m_device, m_initHandles.swapchainImageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_initHandles.swapchain, nullptr);

        CreateSwapchain(m_device, m_initHandles, windowWidth, windowHeight, m_graphicsQueue, 
        m_presentQueue, m_computeQueue, m_pCustomAllocator);

        //The draw extent should also be updated depending on if the swapchain got bigger or smaller
        m_drawExtent.width = std::min(static_cast<uint32_t>(windowWidth), 
        static_cast<uint32_t>(m_colorAttachment.extent.width));
        m_drawExtent.height = std::min(static_cast<uint32_t>(windowHeight), 
        static_cast<uint32_t>(m_colorAttachment.extent.height));
    }
}