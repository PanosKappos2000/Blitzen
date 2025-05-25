#if defined(linux)

#include "Platform/blitPlatformContext.h"
#include "Renderer/BlitzenVulkan/vulkanData.h"
#include <vulkan/vulkan_xcb.h>

namespace BlitzenPlatform
{
	void UNSET_VALIDATION_LAYER_LENS()
	{
		//unsetenv("VK_INSTANCE_LAYERS");
	}

	uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator, void* pPlatform)
    {
        auto P_HANDLE{ reinterpret_cast<PlatformContext*>(pPlatform) };

        VkXcbSurfaceCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        info.connection = P_HANDLE->m_pConnection;
        info.window = P_HANDLE->m_window;

        VkResult res = vkCreateXcbSurfaceKHR(instance, &info, pAllocator, &surface);
        if (res != VK_SUCCESS)
		{
			BLIT_ERROR("Failed to create vulkan surface");
	        return 0;
		}

        return 1;
    }
}

#endif