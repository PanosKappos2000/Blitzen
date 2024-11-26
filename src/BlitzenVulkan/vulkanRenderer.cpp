#include "vulkanRenderer.h"
#include "Platform/platform.h"

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

            uint32_t extensionsCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
            BLIT_ASSERT(50 > extensionsCount)
            VkExtensionProperties availableExtensions[50];
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions);

            // Creating an array of required extension names to pass to ppEnabledExtensionNames
            const char* requiredExtensionNames [BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT];
            requiredExtensionNames[0] =  VULKAN_SURFACE_KHR_EXTENSION_NAME;         
            instanceInfo.enabledExtensionCount = BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT;
            //If this is a debug build, the validation layer extension is also needed
            #ifndef NDEBUG
                requiredExtensionNames[1] = "VK_EXT_debug_utils";
            #endif
            instanceInfo.ppEnabledExtensionNames = requiredExtensionNames;

            /*TODO: enable validation layers
            instanceInfo.enabledLayerCount = ? */
             
            instanceInfo.pApplicationInfo = &applicationInfo;

            VkResult res = vkCreateInstance(&instanceInfo, m_pCustomAllocator, &(m_initHandles.instance));
            VK_CHECK(vkCreateInstance(&instanceInfo, m_pCustomAllocator, &(m_initHandles.instance)));
        }
        /*--------------------------------------------------
            VkInstance created and stored in m_initHandles
        ----------------------------------------------------*/
    }

    void VulkanRenderer::Shutdown()
    {
        vkDestroyInstance(m_initHandles.instance, m_pCustomAllocator);
    }
}