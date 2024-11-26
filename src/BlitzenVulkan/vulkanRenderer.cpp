#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
    void VulkanRenderer::Init()
    {
        /*
            If Blitzen ever uses a custom allocator it should be initalized here
        */
        m_pCustomAllocator = nullptr;

        /*------------------------
            VkInstance Creation
        -------------------------*/
        {
            //Will be passed to the VkInstanceCreateInfo that will create Vulkan's instance
            VkApplicationInfo applicationInfo{};
            applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            applicationInfo.pNext = nullptr; // Not using this
            applicationInfo.apiVersion = VK_API_VERSION_1_3; // There are some features and extensions Blitzen will use, that exist in Vulkan 1.3
            // These are not important right now but passing them either way, to be thorough
            applicationInfo.pApplicationName = BLITZEN_VULKAN_USER_APPLICATION;
            applicationInfo.applicationVersion = BLITZEN_VULKAN_USER_APPLICATION_VERSION;
            applicationInfo.pEngineName = BLITZEN_VULKAN_USER_ENGINE;
            applicationInfo.engineVersion = BLITZEN_VULKAN_USER_ENGINE_VERSION;

            VkInstanceCreateInfo instanceInfo {};
            instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceInfo.pNext = nullptr; // Not using this
            instanceInfo.flags = 0; // Not using this 
            /* 
            TODO : specify the enabled extensions needed and validation layers
            instanceInfo.enabledExtensionCount = ? 
            instanceInfo.enabledLayerCount = ? 
            */ 
            instanceInfo.pApplicationInfo = &applicationInfo;

            //Finally Creation the instance
            vkCreateInstance(&instanceInfo, m_pCustomAllocator, &(m_initHandles.instance));
        }
        /*--------------------------------------------------
            VkInstance created and stored in m_initHandles
        ----------------------------------------------------*/
    }

    VulkanRenderer::~VulkanRenderer()
    {
        vkDestroyInstance(m_initHandles.instance, m_pCustomAllocator);
    }
}