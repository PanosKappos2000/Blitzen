#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
    uint8_t VulkanRenderer::FrameTools::Init(VkDevice device, Queue graphicsQueue, Queue transferQueue, Queue computeQueue)
    {
        // Main command buffer
        VkCommandPoolCreateInfo mainCommandPoolInfo {};
        mainCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        mainCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        mainCommandPoolInfo.queueFamilyIndex = graphicsQueue.index;
        if (vkCreateCommandPool(device, &mainCommandPoolInfo, nullptr, &mainCommandPool.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create command pool for main command buffer");
            return 0;
        }
        VkCommandBufferAllocateInfo mainCommandBufferInfo{};
        mainCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        mainCommandBufferInfo.pNext = nullptr;
        mainCommandBufferInfo.commandBufferCount = 1;
        mainCommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        mainCommandBufferInfo.commandPool = mainCommandPool.handle;
        if (vkAllocateCommandBuffers(device, &mainCommandBufferInfo, &commandBuffer) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create main command buffer");
            return 0;
        }

        // Dedicated transfer command buffer
		VkCommandPoolCreateInfo dedicatedCommandPoolsInfo{};
		dedicatedCommandPoolsInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		dedicatedCommandPoolsInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        dedicatedCommandPoolsInfo.queueFamilyIndex = transferQueue.index;
        if (vkCreateCommandPool(device, &dedicatedCommandPoolsInfo, nullptr, &transferCommandPool.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create dedicated transfer command buffer pool");
            return 0;
        }
        VkCommandBufferAllocateInfo dedicatedCmbInfo{};
		dedicatedCmbInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		dedicatedCmbInfo.pNext = nullptr;
		dedicatedCmbInfo.commandBufferCount = 1;
		dedicatedCmbInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        dedicatedCmbInfo.commandPool = transferCommandPool.handle;
        if (vkAllocateCommandBuffers(device, &dedicatedCmbInfo, &transferCommandBuffer) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create dedicated transfer command buffer");
            return 0;
        }

        if (BlitzenEngine::Ce_BuildClusters)
        {
            // Dedicated compute command buffer
            VkCommandPoolCreateInfo computeCommandPoolInfo{};
            computeCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            computeCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            computeCommandPoolInfo.queueFamilyIndex = computeQueue.index;
            if (vkCreateCommandPool(device, &computeCommandPoolInfo, nullptr, &computeCommandPool.handle) != VK_SUCCESS)
            {
                BLIT_ERROR("Failed to create compute dedicated command buffer pool");
                return 0;
            }
            VkCommandBufferAllocateInfo computeDedicateCmbInfo{};
            computeDedicateCmbInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            computeDedicateCmbInfo.pNext = nullptr;
            computeDedicateCmbInfo.commandBufferCount = Ce_SinglePointer;
            computeDedicateCmbInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            computeDedicateCmbInfo.commandPool = computeCommandPool.handle;
            if (vkAllocateCommandBuffers(device, &computeDedicateCmbInfo, &computeCommandBuffer) != VK_SUCCESS)
            {
                BLIT_ERROR("Failed to create compute dedicated command buffer");
                return 0;
            }
        }

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceInfo.pNext = nullptr;
        if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create fence");
            return 0;
        }
        if (BlitzenEngine::Ce_BuildClusters)
        {
            VkFenceCreateInfo notSignaledFenceInfo{};
            notSignaledFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            if (vkCreateFence(device, &notSignaledFenceInfo, nullptr, &preCulsterCullingFence.handle) != VK_SUCCESS)
            {
                BLIT_ERROR("Failed to create pre culster culling fence");
                return 0;
            }
        }

        VkSemaphoreCreateInfo semaphoresInfo{};
        semaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoresInfo.flags = 0;
        semaphoresInfo.pNext = nullptr;
        if (vkCreateSemaphore(device, &semaphoresInfo, nullptr, &imageAcquiredSemaphore.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for swapchain image acquire");
            return 0;
        }
        if (vkCreateSemaphore(device, &semaphoresInfo, nullptr, &readyToPresentSemaphore.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for presentation");
            return 0;
        }
        if (vkCreateSemaphore(device, &semaphoresInfo, nullptr, &buffersReadySemaphore.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for var buffer data copy");
            return 0;
        }
        if (BlitzenEngine::Ce_BuildClusters && 
            vkCreateSemaphore(device, &semaphoresInfo, nullptr, &preClusterCullingDoneSemaphore.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for pre cluster culling");
            return 0;
        }

        // Success
        return 1;
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

    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer,
        uint32_t waitSemaphoreCount /* =0 */, VkSemaphoreSubmitInfo* waitSemaphore /* =nullptr */,
        uint32_t signalSemaphoreCount /* =0 */, VkSemaphoreSubmitInfo* signalSemaphore /* =nullptr */,
        VkFence fence /* =VK_NULL_HANDLE */)
    {
        vkEndCommandBuffer(commandBuffer);

        VkCommandBufferSubmitInfo commandBufferInfo{};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferInfo.commandBuffer = commandBuffer;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &commandBufferInfo;

        submitInfo.waitSemaphoreInfoCount = waitSemaphoreCount;
        submitInfo.pWaitSemaphoreInfos = waitSemaphore;

        submitInfo.signalSemaphoreInfoCount = signalSemaphoreCount;
        submitInfo.pSignalSemaphoreInfos = signalSemaphore;

        VK_CHECK(vkQueueSubmit2(queue, 1, &submitInfo, fence))
    }

    void CreateSemahoreSubmitInfo(VkSemaphoreSubmitInfo& semaphoreInfo,
        VkSemaphore semaphore, VkPipelineStageFlags2 stage)
    {
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        semaphoreInfo.pNext = nullptr;
        semaphoreInfo.semaphore = semaphore;
        semaphoreInfo.stageMask = stage;
    }

    void CreateRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, VkImageView imageView, VkImageLayout imageLayout,
        VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearColorValue clearValueColor, VkClearDepthStencilValue clearValueDepth)
    {
        attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachmentInfo.pNext = nullptr;
        attachmentInfo.imageView = imageView;
        attachmentInfo.imageLayout = imageLayout;
        attachmentInfo.loadOp = loadOp;
        attachmentInfo.storeOp = storeOp;
        attachmentInfo.clearValue.color = clearValueColor;
        attachmentInfo.clearValue.depthStencil = clearValueDepth;
    }
}