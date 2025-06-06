#include "vulkanRenderer.h"
#include "vulkanCommands.h"

namespace BlitzenVulkan
{
    uint8_t CommandContext::Init(VkDevice device, Queue graphicsQueue, Queue transferQueue, Queue computeQueue)
    {
        // MAIN GRAPHICS POOL
        VkCommandPoolCreateInfo mainCommandPoolInfo {};
        CreateCommandPoolInfo(mainCommandPoolInfo, graphicsQueue.index, nullptr);
        VkResult graphicsCmdPoolRes{ vkCreateCommandPool(device, &mainCommandPoolInfo, nullptr, &m_mainGraphicsCmdPool.handle) };
        if (graphicsCmdPoolRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create command pool for main command buffer");
            return VK_LOG_ERROR_MSG_AND_RETURN(graphicsCmdPoolRes);
        }

        // MAIN GRAPHICS CMDB
        VkCommandBufferAllocateInfo mainCommandBufferInfo{};
        CreateCmdbInfo(mainCommandBufferInfo, m_mainGraphicsCmdPool.handle);
        VkResult mainGraphicsCmdBResult{ vkAllocateCommandBuffers(device, &mainCommandBufferInfo, &m_mainGraphicsCmdB) };
        if (mainGraphicsCmdBResult != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create main command buffer");
            return VK_LOG_ERROR_MSG_AND_RETURN(mainGraphicsCmdBResult);
        }

        // TRANSFER POOL
		VkCommandPoolCreateInfo dedicatedCommandPoolsInfo{};
        CreateCommandPoolInfo(dedicatedCommandPoolsInfo, transferQueue.index, nullptr);
        VkResult transferPoolRes{ vkCreateCommandPool(device, &dedicatedCommandPoolsInfo, nullptr, &m_transferCmdPool.handle) };
        if (transferPoolRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create dedicated transfer command buffer pool");
            return VK_LOG_ERROR_MSG_AND_RETURN(transferPoolRes);
        }

        // TRANSFER CMDB
        VkCommandBufferAllocateInfo dedicatedCmbInfo{};
        CreateCmdbInfo(dedicatedCmbInfo, m_transferCmdPool.handle);
        VkResult transferCmdBRes{ vkAllocateCommandBuffers(device, &dedicatedCmbInfo, &m_transferCmdB) };
        if (transferCmdBRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create dedicated transfer command buffer");
            return VK_LOG_ERROR_MSG_AND_RETURN(transferCmdBRes);
        }

        // COMPUTE POOL
        VkCommandPoolCreateInfo computeCommandPoolInfo{};
        CreateCommandPoolInfo(computeCommandPoolInfo, computeQueue.index, nullptr);
        VkResult computePoolRes{ vkCreateCommandPool(device, &computeCommandPoolInfo, nullptr, &m_computeCmdPool.handle) };
        if (computePoolRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create compute dedicated command buffer pool");
            return VK_LOG_ERROR_MSG_AND_RETURN(computePoolRes);
        }

        // COMPUTE CMDB
        VkCommandBufferAllocateInfo computeDedicateCmbInfo{};
        CreateCmdbInfo(computeDedicateCmbInfo, m_computeCmdPool.handle);
        VkResult computeCmdBRes{ vkAllocateCommandBuffers(device, &computeDedicateCmbInfo, &m_computeCmdB) };
        if (computeCmdBRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create compute dedicated command buffer");
            return VK_LOG_ERROR_MSG_AND_RETURN(computeCmdBRes);
        }

        // UI GRAPHICS POOL
        VkCommandPoolCreateInfo uiGraphicsPoolInfo{};
        CreateCommandPoolInfo(uiGraphicsPoolInfo, graphicsQueue.index, nullptr);
        VkResult uiPoolRes{ vkCreateCommandPool(device, &uiGraphicsPoolInfo, nullptr, &m_uiGraphicsCmdPool.handle) };
        if (uiPoolRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create ui graphics command pool");
            return VK_LOG_ERROR_MSG_AND_RETURN(uiPoolRes);
        }

        // UI GRAPHICS CMDB
        VkCommandBufferAllocateInfo uiGraphicsCmdBInfo{};
        CreateCmdbInfo(uiGraphicsCmdBInfo, m_uiGraphicsCmdPool.handle);
        VkResult uiCmdbRes{ vkAllocateCommandBuffers(device, &uiGraphicsCmdBInfo, &m_uiGraphicsCmdBuffer) };
        if (uiCmdbRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create ui graphics command buffer");
            return VK_LOG_ERROR_MSG_AND_RETURN(uiCmdbRes);
        }

        // FRAME FENCE
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceInfo.pNext = nullptr;
        VkResult frameFenceRes{ vkCreateFence(device, &fenceInfo, nullptr, &m_frameFence.handle) };
        if (frameFenceRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create fence");
            return VK_LOG_ERROR_MSG_AND_RETURN(frameFenceRes);
        }

        VkResult uiFenceRes{ vkCreateFence(device, &fenceInfo, nullptr, &m_uiFence.handle) };
        if (uiFenceRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create ui fence");
            return 0;
        }

        // CLUSTER DISPATCH FENCE
        VkFenceCreateInfo notSignaledFenceInfo{};
        notSignaledFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VkResult clusterFenceRes{ vkCreateFence(device, &notSignaledFenceInfo, nullptr, &m_preClusterFence.handle) };
        if (clusterFenceRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create pre culster culling fence");
            return VK_LOG_ERROR_MSG_AND_RETURN(clusterFenceRes);
        }

        
        VkSemaphoreCreateInfo semaphoresInfo{};
        semaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoresInfo.flags = 0;
        semaphoresInfo.pNext = nullptr;

        // SWAPCHAIN IMAGE SEMAPHORE
        VkResult swapchainSeamphoreRes{ vkCreateSemaphore(device, &semaphoresInfo, nullptr, &m_swapchainSemaphore.handle) };
        if (swapchainSeamphoreRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for swapchain image acquire");
            return VK_LOG_ERROR_MSG_AND_RETURN(swapchainSeamphoreRes);
        }

        // RENDER SEMAPHORE
        VkResult renderSemaphoreRes{ vkCreateSemaphore(device, &semaphoresInfo, nullptr, &m_renderSemaphore.handle) };
        if (renderSemaphoreRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for presentation");
            return VK_LOG_ERROR_MSG_AND_RETURN(renderSemaphoreRes);
        }

        // EDITOR SEMAPHORE
        VkResult dasherRenderSemaphoreResult{ vkCreateSemaphore(device, &semaphoresInfo, nullptr, &m_dasherRenderSemaphore.handle) };
        if (dasherRenderSemaphoreResult != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create editor render semahore");
            return VK_LOG_ERROR_MSG_AND_RETURN(dasherRenderSemaphoreResult);
        }

        // BUFFER COPY SEMAPHORE
        VkResult bufferCopySemaphoreRes{ vkCreateSemaphore(device, &semaphoresInfo, nullptr, &m_bufferUpdateSemaphore.handle) };
        if (bufferCopySemaphoreRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for var buffer data copy");
            return VK_LOG_ERROR_MSG_AND_RETURN(bufferCopySemaphoreRes);
        }

        // CLUSTER SEMAPHORE
        VkResult clusterSemaphoreRes{ vkCreateSemaphore(device, &semaphoresInfo, nullptr, &m_clusterSemaphore.handle) };
        if (clusterSemaphoreRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for pre cluster culling");
            return VK_LOG_ERROR_MSG_AND_RETURN(clusterSemaphoreRes);
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
        
        VK_CHECK_MSG(vkBeginCommandBuffer(commandBuffer, &commandBufferInfo));
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

        VK_CHECK_MSG(vkQueueSubmit2(queue, 1, &submitInfo, fence));
    }

    void CreateSemahoreSubmitInfo(VkSemaphoreSubmitInfo& semaphoreInfo, VkSemaphore semaphore, VkPipelineStageFlags2 stage)
    {
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        semaphoreInfo.pNext = nullptr;
        semaphoreInfo.semaphore = semaphore;
        semaphoreInfo.stageMask = stage;
    }

    void CreateCommandPoolInfo(VkCommandPoolCreateInfo& cmdPoolInfo, uint32_t queueIndex, void* pNext, 
        VkCommandPoolCreateFlags flags /*VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT*/)
    {
        cmdPoolInfo = {};

        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = flags;
        cmdPoolInfo.queueFamilyIndex = queueIndex;
        cmdPoolInfo.pNext = pNext;
    }

    void CreateCmdbInfo(VkCommandBufferAllocateInfo& cmdbInfo, VkCommandPool cmdbPool)
    {
        cmdbInfo = {};

        cmdbInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdbInfo.commandBufferCount = 1;
        cmdbInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        cmdbInfo.commandPool = cmdbPool;
        cmdbInfo.pNext = nullptr;
    }
}