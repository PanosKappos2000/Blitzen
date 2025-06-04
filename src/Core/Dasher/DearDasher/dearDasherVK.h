#pragma once

#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenIMGUI
{
	class ImguiVK
	{
	public:

		ImguiVK() = default;

		bool Init(BlitzenVulkan::VulkanRenderer* pVk);

	private:

		BlitzenVulkan::DescriptorPool m_descPool{};
	};
}