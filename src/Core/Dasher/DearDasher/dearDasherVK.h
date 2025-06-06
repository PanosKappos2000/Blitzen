#pragma once

#include "dearDasherData.h"
#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenIMGUI
{
	class ImguiVK
	{
	public:

		ImguiVK() = default;

		DEAR_DASHER_RETURN_CODE Init(BlitzenVulkan::VulkanRenderer* pVk);

		void StartFrame();

		void SubmitFrame();

	private:

		BlitzenVulkan::DescriptorPool m_descPool{};
	};
}