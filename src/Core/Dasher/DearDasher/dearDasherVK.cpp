#include "dearDasher.h"
#include "backends/imgui_impl_vulkan.h"
#include "Renderer/BlitzenVulkan/vulkanResourceFunctions.h"
#include "Renderer/BlitzenVulkan/vulkanPipelines.h"
#include "Renderer/BlitzenVulkan/vulkanCommands.h"

namespace BlitzenIMGUI
{
	DEAR_DASHER_RETURN_CODE ImguiVK::Init(BlitzenVulkan::VulkanRenderer* pRenderer)
	{
		m_pVulkan = pRenderer;

		ImGui_ImplVulkan_InitInfo info {};

		info.PipelineCache = VK_NULL_HANDLE;
		info.Allocator = nullptr;

		info.ApiVersion = BlitzenVulkan::Ce_VkApiVersion;
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
			return DEAR_DASHER_RETURN_CODE::VULKAN_HANDLE_CREATION_FAILED;
		}

		info.DescriptorPool = m_descPool.handle;

		info.MinImageCount = pRenderer->m_swapchain.m_minImageCount;
		info.ImageCount = BlitML::Max(BlitzenVulkan::ce_framesInFlight, info.MinImageCount);

		info.UseDynamicRendering = true;
		VkFormat formats[] { VK_FORMAT_UNDEFINED };
		BlitzenVulkan::CreatePipelineRenderingCreateInfoKHR(info.PipelineRenderingCreateInfo, formats);
		
		pRenderer->LendRenderingInfos(m_pColorTargetInfo, m_colorTargetHandle);

		for (uint32_t frame = 0; frame < BlitzenVulkan::ce_framesInFlight; ++frame)
		{
			if (m_pColorTargetInfo[frame] == nullptr)
			{
				BLIT_ERROR("Failed to get color target info vk imgui editor");
				return DEAR_DASHER_RETURN_CODE::VULKAN_COLOR_TARGET_NOT_FOUND;
			}

			if (m_colorTargetHandle[frame] == VK_NULL_HANDLE)
			{
				BLIT_ERROR("Failed to get color target for vk imgui editor");
				return DEAR_DASHER_RETURN_CODE::VULKAN_COLOR_TARGET_NOT_FOUND;
			}
		}

		if (!ImGui_ImplVulkan_Init(&info))
		{
			BLIT_ERROR("Failed to initialize vk imgui ui");
			return DEAR_DASHER_RETURN_CODE::IMGUI_HANDLE_CREATION_FAILED;
		}

		return DEAR_DASHER_RETURN_CODE::SUCCESS;
	}

	void ImguiDrawEditor(BlitzenIMGUI::ImguiVK& imguiVk, ImGuiIO& io, float deltaTime)
	{
		ImGui_ImplVulkan_NewFrame();
	}

	void ImguiStartRecording(ImguiVK& imguiVk)
	{
		auto& cmd = imguiVk.m_pVulkan->m_commandsContext[imguiVk.m_pVulkan->m_currentFrame];
		auto pColorTarget{ imguiVk.m_pColorTargetInfo[imguiVk.m_pVulkan->m_currentFrame] };

		vkWaitForFences(imguiVk.m_pVulkan->m_device, 1, &cmd.m_uiFence.handle, VK_TRUE, BlitzenVulkan::ce_fenceTimeout);
		VK_CHECK_MSG(vkResetFences(imguiVk.m_pVulkan->m_device, 1, &cmd.m_uiFence.handle));

		BlitzenVulkan::BeginCommandBuffer(cmd.m_uiGraphicsCmdBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	}

	void ImguiBeginRenderPass(ImguiVK& imguiVk)
	{
		auto& cmd = imguiVk.m_pVulkan->m_commandsContext[imguiVk.m_pVulkan->m_currentFrame];
		auto pColorTarget{ imguiVk.m_pColorTargetInfo[imguiVk.m_pVulkan->m_currentFrame] };

		pColorTarget->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

		BlitzenVulkan::FirstColorPassBarriers(cmd.m_uiGraphicsCmdBuffer, imguiVk.m_colorTargetHandle[imguiVk.m_pVulkan->m_currentFrame]);

		BlitzenVulkan::BeginRendering(cmd.m_uiGraphicsCmdBuffer, VkExtent2D{ imguiVk.m_pVulkan->m_drawWidth, imguiVk.m_pVulkan->m_drawHeight },
			{ 0, 0 }, 1, pColorTarget, nullptr, nullptr);
	}

	void ImguiSubmitEditorRender(ImguiVK& imguiVk)
	{
		auto& cmd = imguiVk.m_pVulkan->m_commandsContext[imguiVk.m_pVulkan->m_currentFrame];
		auto pColorTarget{ imguiVk.m_pColorTargetInfo[imguiVk.m_pVulkan->m_currentFrame] };

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd.m_uiGraphicsCmdBuffer);

		vkCmdEndRendering(cmd.m_uiGraphicsCmdBuffer);

		imguiVk.m_pVulkan->CopyTargetToSwapchain(cmd.m_uiGraphicsCmdBuffer);

		VkSemaphoreSubmitInfo waitSubmitInfo{};
		BlitzenVulkan::CreateSemahoreSubmitInfo(waitSubmitInfo, cmd.m_dasherRenderSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

		BlitzenVulkan::SubmitCommandBuffer(imguiVk.m_pVulkan->m_graphicsQueue.handle, cmd.m_uiGraphicsCmdBuffer, 0, nullptr, 1, &waitSubmitInfo, cmd.m_uiFence.handle);
	}

	void ImguiUpdateEditorWindow(ImguiVK& imguiVk)
	{
		imguiVk.m_pVulkan->LendRenderingInfos(imguiVk.m_pColorTargetInfo, imguiVk.m_colorTargetHandle);
	}

	ImguiVK::~ImguiVK()
	{
		vkDeviceWaitIdle(m_pVulkan->m_device);

		//ImGui_ImplVulkan_InvalidateDeviceObjects();
	}
}