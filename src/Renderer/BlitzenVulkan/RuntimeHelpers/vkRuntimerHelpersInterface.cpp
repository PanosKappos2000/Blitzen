#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenEngine
{
	void PrepareRendererForRuntime(BlitzenVulkan::VulkanRenderer* pRenderer)
	{
		vkDeviceWaitIdle(pRenderer->m_device);
	}
}