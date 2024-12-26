#pragma once

/*#define VK_NO_PROTOTYPES
#include <Volk/volk.h>*/
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "Core/blitzenContainerLibrary.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Application/resourceLoading.h"

// Right now I don't know if I should rely on this or my own math library
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

#define BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT     1 /* This is used for double(+) buffering, I activated it back, 
but I should probably create a second indirect buffer because of it */

#define BLITZEN_VULKAN_INDIRECT_DRAW            1
#define BLITZEN_VULKAN_MESH_SHADER              0// Mesh shader implementation is pretty terrible at the moment 

#define BLITZEN_VULKAN_MAX_DRAW_CALLS           100000 // I am ignoring this right now and I shouldn't be

#define BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT     2 + BLITZEN_VULKAN_VALIDATION_LAYERS

namespace BlitzenVulkan
{
    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;// If a discrete GPU is found, it will be chosen
        uint8_t meshShaderSupport = 0;
    };

    // Holds the data of a static object. Will be passed to the shaders only once during loading and will be indexed in the shaders
    struct alignas (16) StaticRenderObject
    {
        BlitML::vec3 pos;
        float scale;
        BlitML::quat orientation;
        BlitML::vec3 center;
	    float radius;

        uint32_t materialTag;
    };

    struct alignas(16) IndirectOffsets
    {
        BlitzenEngine::MeshLod lod[8];
        uint32_t lodCount;

        uint32_t vertexOffset;

        uint32_t taskCount;
        uint32_t firstTask;
    };

    struct alignas(16) IndirectDrawData
    {
        uint32_t drawId;
        VkDrawIndexedIndirectCommand drawIndirect;// 5 32bit integers
        VkDrawMeshTasksIndirectCommandNV drawIndirectTasks;// 2 32bit integers
    };

    // Holds everything that needs to be given to the renderer during load and converted to data that will be used by the GPU when drawing a frame
    struct GPUData
    {
        BlitCL::DynamicArray<BlitML::Vertex>& vertices;

        BlitCL::DynamicArray<uint32_t>& indices;

        BlitCL::DynamicArray<BlitML::Meshlet>& meshlets;

        BlitzenEngine::MeshAssets* pMeshes;
        size_t meshCount;

        BlitzenEngine::TextureStats* pTextures; 
        size_t textureCount;

        BlitzenEngine::MaterialStats* pMaterials;
        size_t materialCount;

        inline GPUData(BlitCL::DynamicArray<BlitML::Vertex>& v, BlitCL::DynamicArray<uint32_t>& i, BlitCL::DynamicArray<BlitML::Meshlet>& m)
            :vertices(v), indices(i), meshlets(m)
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

        size_t drawCount;

        BlitML::vec3 sunlightDirection;
        BlitML::vec4 sunlightColor;

        uint8_t debugPyramid = 0;
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

    // Holds universal, shader data that will be passed at the beginning of each frame to the shaders as a uniform buffer
    struct alignas(16) GlobalShaderData
    {
        // Accessed by a compute shader to do frustum culling
        BlitML::vec4 frustumData[6];

        BlitML::mat4 projection;
        BlitML::mat4 view;
        BlitML::mat4 projectionView;

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
        VkDeviceAddress indirectBufferAddress;
        VkDeviceAddress finalIndirectBufferAddress;
        VkDeviceAddress indirectCountBufferAddress;
        VkDeviceAddress visibilityBufferAddress;
    };

    // Pushed every frame for the non indirect version to access per object data
    struct alignas(16) ShaderPushConstant
    {
        BlitML::vec2 imageSize;
    };

    // Passed to the shaders through a storage buffer that contains all of these
    struct alignas(16) MaterialConstants
    {
        BlitML::vec4 diffuseColor;
        float shininess;

        uint32_t diffuseTextureTag;
        uint32_t specularTextureTag;
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