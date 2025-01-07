#pragma once

/*#define VK_NO_PROTOTYPES
#include <Volk/volk.h>*/
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "Core/blitzenContainerLibrary.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Application/resourceLoading.h"
#include "Game/blitObject.h"

// My math library seems to be fine now but I am keeping this to compare values when needed
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
    #define BLITZEN_VULKAN_VALIDATION_LAYERS                        0
    #define VK_CHECK(expr)                                          expr;
#else
    #define BLITZEN_VULKAN_VALIDATION_LAYERS                        1
    #define VK_CHECK(expr)                                          BLIT_ASSERT(expr == VK_SUCCESS)
#endif

#define BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT     1 // This is used for double(+) buffering

#define BLITZEN_VULKAN_INDIRECT_DRAW            1
#define BLITZEN_VULKAN_MESH_SHADER              0// For now mesh shaders are completely busted 

#define BLITZEN_VULKAN_MAX_DRAW_CALLS           5'000'000 // Going to 6'000'000 causes validation errors, but the renderer can still manage it (tested up to 10'000'000)

#define BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT     2 + BLITZEN_VULKAN_VALIDATION_LAYERS

namespace BlitzenVulkan
{
    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;// If a discrete GPU is found, it will be chosen
        uint8_t meshShaderSupport = 0;
    };

    // Accesses per object data (TODO: Should be something defined in a general rendering file like resourceLoading.h)
    struct RenderObject
    {
        uint32_t meshInstanceId;
        uint32_t surfaceId;
    };

    // Holds the command struct for a call to vkCmdDrawIndexedIndirectCount, as well as a draw Id to access the correct RenderObject
    struct IndirectDrawData
    {
        uint32_t drawId;
        VkDrawIndexedIndirectCommand drawIndirect;// 5 32bit integers
    };

    // Holds the command structu for a call to vkCmdDrawMeshTasksIndirectCountExt, as well as a task Id to access the correct task
    struct IndirectTaskData
    {
        uint32_t taskId;
        VkDrawMeshTasksIndirectCommandEXT drawIndirectTasks;// 3 32bit integers
    };

    // Holds everything that needs to be given to the renderer during load and converted to data that will be used by the GPU when drawing a frame
    struct GPUData
    {
        BlitCL::DynamicArray<BlitML::Vertex>& vertices;

        BlitCL::DynamicArray<uint32_t>& indices;

        BlitCL::DynamicArray<BlitML::Meshlet>& meshlets;

        BlitCL::DynamicArray<BlitzenEngine::GameObject> gameObjects;// The data from this will be transformed to mesh instance and render objects

        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces;

        BlitzenEngine::Mesh* pMeshes;
        size_t meshCount;

        BlitzenEngine::TextureStats* pTextures; 
        size_t textureCount;

        BlitzenEngine::Material* pMaterials;
        size_t materialCount;

        size_t drawCount = 0;

        inline GPUData(BlitCL::DynamicArray<BlitML::Vertex>& v, BlitCL::DynamicArray<uint32_t>& i, BlitCL::DynamicArray<BlitML::Meshlet>& m, 
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& s)
            :vertices(v), indices(i), meshlets(m), surfaces(s)
        {}
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
        BlitML::vec3 viewPosition;
        BlitML::mat4 projectionTranspose;
        float zNear = 1.f;

        size_t drawCount;
        float drawDistance;

        BlitML::vec3 sunlightDirection;
        BlitML::vec4 sunlightColor;

        uint8_t debugPyramid = 0;
        uint8_t occlusionEnabled = 1;
        uint8_t lodEnabled = 1;
    };

    // This is the way Vulkan image resoureces are represented by the Blitzen VulkanRenderer
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

    // Holds universal, shader data that will be passed at the beginning of each frame to the shaders as a uniform buffer
    struct alignas(16) GlobalShaderData
    {
        BlitML::mat4 projectionView;
        BlitML::mat4 view;

        BlitML::vec4 sunlightColor;
        BlitML::vec3 sunlightDir;// directional light direction vector

        BlitML::vec3 viewPosition;
    };

    // This struct will be passed to the GPU as uniform descriptor and will give shaders access to all required buffer
    struct alignas(16) BufferDeviceAddresses
    {
        VkDeviceAddress vertexBufferAddress;

        VkDeviceAddress renderObjectBufferAddress;

        VkDeviceAddress materialBufferAddress;

        VkDeviceAddress meshBufferAddress;

        VkDeviceAddress surfaceBufferAddress;
        VkDeviceAddress meshInstanceBufferAddress;

        VkDeviceAddress finalIndirectBufferAddress;
        VkDeviceAddress indirectCountBufferAddress;
        VkDeviceAddress visibilityBufferAddress;
    };

    struct alignas(16) CullingData
    {
        // frustum planes
        float frustumRight;
        float frustumLeft;
        float frustumTop;
        float frustumBottom;

        float proj0;// The 1st element of the projection matrix
        float proj5;// The 12th element of the projection matrix

        // The draw distance and zNear, needed for both occlusion and frustum culling
        float zNear;
        float zFar;

        // Occulusion culling depth pyramid data
        float pyramidWidth;
        float pyramidHeight;

        // Debug values
        uint32_t occlusionEnabled;
        uint32_t lodEnabled;
    };

    // Pushed every frame for the non indirect version to access per object data
    struct alignas(16) ShaderPushConstant
    {
        BlitML::vec2 imageSize;
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