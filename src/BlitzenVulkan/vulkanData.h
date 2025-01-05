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

#define DESIRED_SWAPCHAIN_PRESENTATION_MODE                     VK_PRESENT_MODE_FIFO_KHR

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

    struct RenderObject
    {
        uint32_t meshInstanceId;
        uint32_t surfaceId;
    };

    // This will be passed to the shaders and indexed into everytime an instance of this surface appears
    struct alignas(16) SurfaceData
    {
        BlitzenEngine::MeshLod meshLod[BLIT_MAX_MESH_LOD];
        uint32_t lodCount = 0;

        // With the way obj files are loaded, this will be needed to index into the vertex buffer
        uint32_t vertexOffset;

        // Data need by a mesh shader to draw a surface
        uint32_t meshletCount = 0;
        uint32_t firstMeshlet;

        // Bounding sphere data, can be used for frustum culling and other operations
        BlitML::vec3 center;
        float radius;

        uint32_t materialTag;

        uint32_t surfaceIndex;
    };

    // There might be multiple instances of a mesh with different transforms, so this data should be different for each instance
    struct alignas(16) MeshInstance
    {
        BlitML::vec3 pos;
        float scale;
        BlitML::quat orientation;
    };

    // This holds the commands for both the mesh shader and vertex shader indirect commands
    struct IndirectDrawData
    {
        uint32_t drawId;
        VkDrawIndexedIndirectCommand drawIndirect;// 5 32bit integers
        //VkDrawMeshTasksIndirectCommandNV drawIndirectTasks;// 2 32bit integers
        //VkDrawMeshTasksIndirectCommandEXT drawIndirectTasks;// 3 32 bit integers
    };

    struct IndirectTaskData
    {
        uint32_t taskId;
        VkDrawMeshTasksIndirectCommandEXT drawIndirectTasks;
    };

    // Holds everything that needs to be given to the renderer during load and converted to data that will be used by the GPU when drawing a frame
    struct GPUData
    {
        BlitCL::DynamicArray<BlitML::Vertex>& vertices;

        BlitCL::DynamicArray<uint32_t>& indices;

        BlitCL::DynamicArray<BlitML::Meshlet>& meshlets;

        BlitCL::DynamicArray<BlitzenEngine::GameObject> gameObjects;// The data from this will be transformed to mesh instance and render objects

        BlitzenEngine::MeshAssets* pMeshes;
        size_t meshCount;

        BlitzenEngine::TextureStats* pTextures; 
        size_t textureCount;

        BlitzenEngine::MaterialStats* pMaterials;
        size_t materialCount;

        size_t drawCount = 0;

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
        float zNear = 1.f;

        size_t drawCount;
        float drawDistance;

        BlitML::vec3 sunlightDirection;
        BlitML::vec4 sunlightColor;

        uint8_t debugPyramid = 0;
        uint8_t occlusionEnabled = 1;
        uint8_t lodEnabled = 1;
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
        BlitML::vec4 frustumData[6];
        float proj0;// The 1st element of the projection matrix
        float proj5;// The 12th element of the projection matrix
        float zNear;
        // Occulusion culling depth pyramid data
        float pyramidWidth;
        float pyramidHeight;
        uint32_t occlusionEnabled;
        uint32_t lodEnabled;
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