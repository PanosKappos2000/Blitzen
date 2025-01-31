#pragma once

/*#define VK_NO_PROTOTYPES
#include <Volk/volk.h>*/

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "Core/blitzenContainerLibrary.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Renderer/blitRenderingResources.h"
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
#define BLITZEN_VULKAN_MESH_SHADER              0 // Mesh shaders have output but it is wrong 

#define BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT     2 + BLITZEN_VULKAN_VALIDATION_LAYERS

namespace BlitzenVulkan
{
    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;// If a discrete GPU is found, it will be chosen
        uint8_t meshShaderSupport = 0;
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
        BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices;

        BlitCL::DynamicArray<uint32_t>& indices;

        BlitCL::DynamicArray<BlitzenEngine::Meshlet>& meshlets;

        BlitCL::DynamicArray<uint32_t>& meshletData;

        BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& transforms;

        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces;

        BlitzenEngine::GameObject* pGameObjects;// This will be transformed to render objects
        size_t gameObjectCount;

        BlitzenEngine::RenderObject* pRenderObjects;
        size_t renderObjectCount;

        BlitzenEngine::Mesh* pMeshes;
        size_t meshCount;

        BlitzenEngine::TextureStats* pTextures; 
        size_t textureCount;

        BlitzenEngine::Material* pMaterials;
        size_t materialCount;

        size_t drawCount = 0;

        inline GPUData(BlitCL::DynamicArray<BlitzenEngine::Vertex>& v, BlitCL::DynamicArray<uint32_t>& i, 
        BlitCL::DynamicArray<BlitzenEngine::Meshlet>& m, BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& s, 
        BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& t, BlitCL::DynamicArray<uint32_t>& md)
            :vertices(v), indices(i), meshlets(m), surfaces(s), transforms(t), meshletData(md)
        {}
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

    // This struct will be passed to the GPU as uniform descriptor and will give shaders access to the global storage buffers
    struct alignas(16) BufferDeviceAddresses
    {
        // Address of the global vertex buffer, accessed in the vertex and mesh shaders
        VkDeviceAddress vertexBufferAddress;

        // Address of per object data buffer, accessed in multiple shaders for per object / per instance indices
        VkDeviceAddress renderObjectBufferAddress;

        // Address of the buffer that holds all material data, accessed in vertex, mesh and fragment shaders to perform shading
        VkDeviceAddress materialBufferAddress;

        // Address of the buffer that holds all meshlets in the scene, accessed in mesh and task shaders
        VkDeviceAddress meshletBufferAddress;

        // Address of the buffer that holds all meshlet indices in the scene, accessed in mesh and task shaders
        VkDeviceAddress meshletDataBufferAddress;

        // Address of the buffer that holds all surfaces in the scene, accessed by many shaders for per unique render object data
        VkDeviceAddress surfaceBufferAddress;

        // Address of the buffer that holds the transforms and other per instance data, accessed by mnay shaders for per instance data
        VkDeviceAddress transformBufferAddress;

        // Holds the indirect draw commands buffer address. It is accessed by the culling compute shaders to be setup before drawing
        VkDeviceAddress indirectDrawBufferAddress;

        // Holds the indirect task commands buffer address. It is accessed by the culling compute shaders to be setup before drawing
        VkDeviceAddress indirectTaskBufferAddress;

        // Holds the indirect count buffer address, that only holds a single integer. 
        // It is accessed by the culling compute shaders to be setup before drawing
        VkDeviceAddress indirectCountBufferAddress;

        // Holds the address of the buffer that holds an integer for the previous frame visibility of every object in the scene. 
        // Accessed by the culling compute shaders
        VkDeviceAddress visibilityBufferAddress;
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