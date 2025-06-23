#pragma once
#include "Renderer/BlitzenVulkan/Context/vulkanContext.h"

namespace BlitzenVulkan
{
    void RecreateSwapchain(VkDevice device, VkInstance instance, Swapchain& swapchainData, VkSurfaceKHR surface, VkPhysicalDevice pdv, VmaAllocator vma,
        PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources* readWrites, DescriptorContext& descriptorContext, uint32_t windowWidth, uint32_t windowHeight, uint32_t frame, Queue graphicsQueue, Queue presentQueue, Queue computeQueue);
}