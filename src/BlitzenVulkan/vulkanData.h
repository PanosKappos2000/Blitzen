#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "Core/blitzenContainerLibrary.h"
#include "BlitzenMathLibrary/blitML.h"

// Temporary to debug my math library
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"

// These macros will be used to initalize VkApplicationInfo which will be passed to VkInstanceCreateInfo
#define BLITZEN_VULKAN_USER_APPLICATION                         "Blitzen Game"
#define BLITZEN_VULKAN_USER_APPLICATION_VERSION                 VK_MAKE_VERSION (1, 0, 0)
#define BLITZEN_VULKAN_USER_ENGINE                              "Blitzen Engine"
#define BLITZEN_VULKAN_USER_ENGINE_VERSION                      VK_MAKE_VERSION (1, 0, 0)

#define DESIRED_SWAPCHAIN_PRESENTATION_MODE                     VK_PRESENT_MODE_MAILBOX_KHR

#ifdef NDEBUG
    #define BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT                  2
    #define VK_CHECK(expr)                                          expr;
#else
    #define BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT                  3
    #define VALIDATION_LAYER_NAME                                   "VK_LAYER_KHRONOS_validation"
    #define VK_CHECK(expr)                                          BLIT_ASSERT(expr == VK_SUCCESS)
#endif

#define BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT     2 
#define BLITZEN_VULKAN_INDIRECT_DRAW            1      

namespace BlitzenVulkan
{
    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;// If a discrete GPU is found, it will be chosen
        uint8_t drawIndirect = 0;
    };

    // Passed to the renderer every time draw frame is called, to handle special events and update shader data
    struct RenderContext
    {
        uint8_t windowResize = 0;
        uint32_t windowWidth;
        uint32_t windowHeight;

        BlitML::mat4 projectionMatrix;
        BlitML::mat4 viewMatrix;
        BlitML::mat4 projectionView;
    };

    struct AllocatedImage
    {
        VkImage image;
        VkImageView imageView;

        VkExtent3D extent;
        VkFormat format;

        VmaAllocation allocation;

        // Implemented in vulkanResoures.cpp
        void CleanupResources(VmaAllocator allocator, VkDevice device);
    };

    struct AllocatedBuffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
    };

    // Holds universal, constant shader data that will be passed at the beginning of each frame to the shaders as a uniform buffer
    struct alignas(16) GlobalShaderData
    {
        BlitML::mat4 projection;
        BlitML::mat4 view;
        BlitML::mat4 projectionView;

        VkDeviceAddress vertexBufferAddress;
        VkDeviceAddress renderObjectBufferAddress;
    };

    // Pushed every frame for the non indirect version to access per object data
    struct alignas(16) ShaderPushConstant
    {
        uint32_t drawTag;
    };

    // Holds the data of a static object. Will be passed to the shader only once during loading and will be indexed in the shaders
    struct alignas (16) StaticRenderObject
    {
        BlitML::mat4 modelMatrix;
        uint32_t textureTag;
    };

    // This will be used to momentarily hold all the textures while loading and then pass them to the descriptor all at once
    struct TextureData
    {
        AllocatedImage image;
        VkSampler sampler;
    };
}