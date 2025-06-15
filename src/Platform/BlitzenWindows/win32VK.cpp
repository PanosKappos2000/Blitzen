#if defined(_WIN32)

#include "Platform/blitPlatformContext.h"
#include "Renderer/BlitzenVulkan/Context/vulkanData.h"
#include <vulkan/vulkan_win32.h>

namespace BlitzenPlatform
{
    uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator, void* pPlatform)
    {
        auto platform{ reinterpret_cast<BlitzenPlatform::PlatformContext*>(pPlatform) };

        VkWin32SurfaceCreateInfoKHR info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        info.hinstance = platform->m_hinstance;
        info.hwnd = platform->m_hwnd;

        auto res = vkCreateWin32SurfaceKHR(instance, &info, pAllocator, &surface);
        if (res != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create Vulkan surface");
            return 0;
        }
        return 1;
    }

    void UNSET_VALIDATION_LAYER_LENS()
    {
        SetEnvironmentVariable("VK_INSTANCE_LAYERS", "");
    }
}

#endif