#pragma once

#include "dearDasherData.h"
#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenIMGUI
{
	class ImguiVK
	{
	public:

		DEAR_DASHER_RETURN_CODE Init(BlitzenVulkan::VulkanRenderer* pVk);

		BlitzenVulkan::DescriptorPool m_descPool{};

		VkRenderingAttachmentInfo* m_pColorTargetInfo[BlitzenVulkan::ce_framesInFlight]{ nullptr };
		VkRenderingAttachmentInfo* m_pDepthTargetInfo[BlitzenVulkan::ce_framesInFlight]{ nullptr };

		BlitzenVulkan::VulkanRenderer* m_pVulkan{ nullptr };

		~ImguiVK();
	};
}