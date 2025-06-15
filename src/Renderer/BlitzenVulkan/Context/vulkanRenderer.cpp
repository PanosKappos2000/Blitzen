#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void VulkanRenderer::LendRenderingInfos(VkRenderingAttachmentInfo** ppColorInfo, VkImage* pColorTarget)
    {
        for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            ppColorInfo[frame] = &m_pipelines.m_colorTargetInfo[frame];
            pColorTarget[frame] = m_readWrites[frame].m_colorTarget.m_image.m_image.m_handle;
        }
    }

    // Few manual destructions remaining, mostly because of my laziness
    VulkanRenderer::~VulkanRenderer()
    {
        // Wait for the device to finish its work before destroying resources
        vkDeviceWaitIdle(m_device);

        if (m_debugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        }
    }
}