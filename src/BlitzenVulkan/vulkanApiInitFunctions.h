#pragma once
#include "vulkanData.h"

namespace BlitzenVulkan
{
    void CreateApplicationInfo(VkApplicationInfo& appInfo, void* pNext, const char* appName, uint32_t appVersion,
        const char* engineName, uint32_t engineVersion, uint32_t apiVersion = VK_API_VERSION_1_3);

    // Tries to find the requested swapchain format, saves the chosen format to the VkFormat ref
    uint8_t FindSwapchainSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, 
        VkSwapchainCreateInfoKHR& info, VkFormat& swapchainFormat);

    // Validates existence of desired present mode
    uint8_t FindSwapchainPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info);

    uint8_t FindSwapchainSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info, Swapchain& swapchain);

}