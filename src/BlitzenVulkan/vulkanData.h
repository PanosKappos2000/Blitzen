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
    #define VK_CHECK(expr)                                          BLIT_ASSERT(expr == VK_SUCCESS)
#endif

#define BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT     2 
#define BLITZEN_VULKAN_INDIRECT_DRAW            1 

#define BLITZEN_VULKAN_MAX_DRAW_CALLS           1000000

namespace BlitzenVulkan
{
    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;// If a discrete GPU is found, it will be chosen
        uint8_t drawIndirect = 0;
    };

    // Holds the data of a static object. Will be passed to the shaders only once during loading and will be indexed in the shaders
    struct alignas (16) StaticRenderObject
    {
        BlitML::mat4 modelMatrix;
        uint32_t materialTag;
    };

    // Holds everything that needs to be passed to the UploadDataToGPUAndSetupForRendering function
    struct GPUData
    {
        BlitCL::DynamicArray<BlitML::Vertex>& vertices;

        BlitCL::DynamicArray<uint32_t>& indices;

        BlitCL::DynamicArray<StaticRenderObject>& staticObjects;

        void* pTextures; 
        size_t textureCount;

        void* pMaterials;
        size_t materialCount;

        inline GPUData(BlitCL::DynamicArray<BlitML::Vertex>& v, BlitCL::DynamicArray<uint32_t>& i, BlitCL::DynamicArray<StaticRenderObject>& o)
            :vertices(v), indices(i), staticObjects(o)
        {}
    };

    // This is setup before a call to draw frame, and is given to the render context in its member array
    struct DrawObject
    {
        // Data used to call draw indexed
        uint32_t firstIndex;
        uint32_t indexCount;

        // References a render object in the global buffer
        uint32_t objectTag;
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

        DrawObject* pDraws;
        size_t drawCount;

        BlitML::vec3 sunlightDirection;
        BlitML::vec4 sunlightColor;
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

        BlitML::vec3 sunlightDir;// directional light direction vector
        BlitML::vec4 sunlightColor;

        VkDeviceAddress vertexBufferAddress;
        VkDeviceAddress renderObjectBufferAddress;
        VkDeviceAddress materialBufferAddress;
    };

    // Pushed every frame for the non indirect version to access per object data
    struct alignas(16) ShaderPushConstant
    {
        uint32_t drawTag;
    };

    // Passed to the shaders through a storage buffer that contains all of these
    struct alignas(16) MaterialConstants
    {
        BlitML::vec4 diffuseColor;
        uint32_t diffuseTextureTag;
    };

    // This will be used to momentarily hold all the textures while loading and then pass them to the descriptor all at once
    struct TextureData
    {
        AllocatedImage image;
        VkSampler sampler;
    };

}

namespace BlitzenPlatform
{
    // Creates the surface used by the vulkan renderer. Implemented on Platform.cpp
    void CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator);
}