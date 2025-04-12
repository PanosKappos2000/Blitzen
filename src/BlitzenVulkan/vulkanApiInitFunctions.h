#pragma once
#include "vulkanData.h"

namespace BlitzenVulkan
{
    // Creates the Vulkan instance, required to interface with the Vulkan API
    uint8_t CreateInstance(VkInstance& instance, VkDebugUtilsMessengerEXT* pDM = nullptr);

    // Checks if the requested validation layers are supported
    uint8_t EnableInstanceValidation(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo);

    uint8_t EnabledInstanceSynchronizationValidation();

    // Picks a suitable physical device (GPU) for the application and passes the handle to the first argument.
    // Returns 1 if if finds a fitting device. Expects the instance and surface arguments to be valid
    uint8_t PickPhysicalDevice(VkPhysicalDevice& gpu, VkInstance instance, VkSurfaceKHR surface,
		Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue,
        VulkanStats& stats
    );

    // Validates the suitability of a GPU for the application
    uint8_t ValidatePhysicalDevice(VkPhysicalDevice pdv, VkInstance instance, VkSurfaceKHR surface, 
		Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue,
        VulkanStats& stats
    );

    // Looks for extensions in a physical device, returns 1 if everything goes well, and uploads data to stats and the extension count
    uint8_t LookForRequestedExtensions(VkPhysicalDevice gpu, VulkanStats& stats);

    // Creates the logical device, after activating required extensions and features
    uint8_t CreateDevice(VkDevice& device, VkPhysicalDevice physicalDevice, Queue& graphicsQueue, 
        Queue& presentQueue, Queue& computeQueue, Queue& transferQueue, VulkanStats& stats
    );

    // Tries to find the requested swapchain format, saves the chosen format to the VkFormat ref
    uint8_t FindSwapchainSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, 
        VkSwapchainCreateInfoKHR& info, VkFormat& swapchainFormat
    );

    uint8_t FindSwapchainPresentMode(VkPhysicalDevice physicalDevice, 
        VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info
    );

    uint8_t FindSwapchainSurfaceCapabilities(VkPhysicalDevice physicalDevice, 
        VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info, Swapchain& swapchain
    );

}