#include "vulkanRenderer.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/DbLog/blitLogger.h"

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

    void VK_RES_MSG_ASSRT(VkResult res)
    {
        BLIT_ASSERT_MESSAGE(res == VK_SUCCESS, VK_TRANS_RES(res));
    }

    uint8_t VK_LOG_ERROR_MSG_AND_RETURN(VkResult res)
    {
        if (res < 0)
        {
            BLIT_ERROR("VKRESULT WITH: %s", VK_TRANS_RES(res));
            return 0;
        }

        BLIT_WARN("No error message found");
        return 0;
    }
}