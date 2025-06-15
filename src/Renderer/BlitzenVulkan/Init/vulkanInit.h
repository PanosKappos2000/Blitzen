#pragma once
#include "Renderer/BlitzenVulkan/Context/vulkanContext.h"

namespace BlitzenVulkan
{
	uint8_t CreateInstance(VkInstance& instance, VulkanStats& stats, VkDebugUtilsMessengerEXT* pDM = nullptr);

	uint8_t PickPhysicalDevice(VkPhysicalDevice& gpu, VkInstance instance, VkSurfaceKHR surface, Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue, VulkanStats& stats);

	uint8_t ValidatePhysicalDevice(VkPhysicalDevice pdv, VkInstance instance, VkSurfaceKHR surface, Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue, VulkanStats& stats);

	void GetVulkanQueue(VkDevice device, Queue& queue, void* pNext, VkDeviceQueueCreateFlags flags, uint32_t queueIndex = 0);

	uint8_t CreateDevice(VkDevice& device, VkPhysicalDevice physicalDevice, Queue& graphicsQueue, Queue& presentQueue, Queue& computeQueue, Queue& transferQueue, VulkanStats& stats);

	uint8_t SetupResourceManagement(VkDevice device, VkPhysicalDevice pdv, VkInstance instance, VmaAllocator& vma, MemoryCrucialHandles& memoryCrucials);

	uint8_t CreateDescriptorLayouts(VkDevice device, DescriptorContext& descriptorContext, VulkanStats& stats, uint32_t textureCount);

	uint8_t CreatePipelineLayouts(VkDevice device, PipelineContext& context, DescriptorContext& descriptorContext);

	uint8_t CreateReadWriteBuffers(VkDevice device, VmaAllocator vma, RWResources* readWritesArray, DescriptorContext& descriptorContext);

	uint8_t CreateReadOnlyBuffers(VkDevice device, VmaAllocator vma, ROResources& readOnlies, VulkanStats& stats);
}