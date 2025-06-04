#include "dearDasherVK.h"
#include "ImGui.h"
#include "backends/imgui_impl_vulkan.h"
#include "Renderer/BlitzenVulkan/vulkanResourceFunctions.h"

namespace BlitzenIMGUI
{
	bool ImguiVK::Init(BlitzenVulkan::VulkanRenderer* pRenderer)
	{
		ImGui_ImplVulkan_InitInfo info {};

		info.PipelineCache = VK_NULL_HANDLE;
		info.Allocator = nullptr;

		info.Instance = pRenderer->m_instance;
		info.PhysicalDevice = pRenderer->m_physicalDevice;
		info.Device = pRenderer->m_device; 

		info.QueueFamily = pRenderer->m_graphicsQueue.index; 
		info.Queue = pRenderer->m_graphicsQueue.handle;

		VkDescriptorPoolCreateInfo poolInfo{};

		VkDescriptorPoolSize poolSize{};
		poolSize.descriptorCount = 100;
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		m_descPool.handle = BlitzenVulkan::CreateDescriptorPool(info.Device, 1, &poolSize, 1);
		if (m_descPool.handle == VK_NULL_HANDLE)
		{
			BLIT_ERROR("Failed to create imguiVK descriptor pool");
			return false;
		}

		info.DescriptorPool = m_descPool.handle;

		info.MinImageCount = 2;

		if (!ImGui_ImplVulkan_Init(&info))
		{
			BLIT_ERROR("Failed to initialize vk imgui ui");
			return false;
		}

		return true;
	}
}