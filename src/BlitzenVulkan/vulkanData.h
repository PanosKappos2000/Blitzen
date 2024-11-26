#pragma once

#include <vulkan/vulkan.h>
#include "Core/blitzenContainerLibrary.h"

// These macros will be used to initalize VkApplicationInfo which will be passed to VkInstanceCreateInfo
#define BLITZEN_VULKAN_USER_APPLICATION                         "Blitzen Game"
#define BLITZEN_VULKAN_USER_APPLICATION_VERSION                 VK_MAKE_VERSION (1, 0, 0)
#define BLITZEN_VULKAN_USER_ENGINE                              "Blitzen Engine"
#define BLITZEN_VULKAN_USER_ENGINE_VERSION                      VK_MAKE_VERSION (1, 0, 0)

#ifdef NDEBUG
    #define BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT                  1
    #define VK_CHECK(expr)                                          expr;
#else
    #define BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT                  2
    #define VALIDATION_LAYER_NAME                                   "VK_LAYER_KHRONOS_validation"
    #define VK_CHECK(expr)                                          BLIT_ASSERT(expr == VK_SUCCESS)
#endif

#define BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT 2       

namespace BlitzenVulkan
{

}