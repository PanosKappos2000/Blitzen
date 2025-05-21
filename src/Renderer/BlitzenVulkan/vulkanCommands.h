#pragma once
#include "vulkanData.h"

namespace BlitzenVulkan
{
    // Puts command buffer in the ready state. vkCmd type function can be called after this and until vkEndCommandBuffer is called
    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags);

    // Creates semaphore submit info which can be passed to VkSubmitInfo2 before queue submit
    void CreateSemahoreSubmitInfo(VkSemaphoreSubmitInfo& semaphoreInfo, VkSemaphore semaphore, VkPipelineStageFlags2 stage);

    // Ends command buffer and submits it. Synchronization structures can also be specified
    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, uint32_t waitSemaphoreCount = 0,
        VkSemaphoreSubmitInfo* pWaitInfo = nullptr, uint32_t signalSemaphoreCount = 0,
        VkSemaphoreSubmitInfo* signalSemaphore = nullptr, VkFence fence = VK_NULL_HANDLE);

    void CreateCommandPoolInfo(VkCommandPoolCreateInfo& cmdPoolInfo, uint32_t queueIndex, void* pNext,
        VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    void CreateCmdbInfo(VkCommandBufferAllocateInfo& cmdbInfo, VkCommandPool cmdbPool);
}