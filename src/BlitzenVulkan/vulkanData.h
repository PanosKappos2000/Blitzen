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

#ifdef BLIT_VSYNC
    #define DESIRED_SWAPCHAIN_PRESENTATION_MODE                     VK_PRESENT_MODE_FIFO_KHR
#else
    #define DESIRED_SWAPCHAIN_PRESENTATION_MODE                     VK_PRESENT_MODE_MAILBOX_KHR
#endif

#ifdef NDEBUG
    #define BLITZEN_VULKAN_VALIDATION_LAYERS                        0
    #define VK_CHECK(expr)                                          expr;
#else
    #define BLITZEN_VULKAN_VALIDATION_LAYERS                        1
    #define VK_CHECK(expr)                                          BLIT_ASSERT(expr == VK_SUCCESS)
#endif

#ifdef BLIT_DOUBLE_BUFFERING
    #define BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT     2
#else
    #define BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT     1 // This is used for double(+) buffering
#endif

#ifdef  BLIT_VK_MESH_EXT
    #define BLITZEN_VULKAN_MESH_SHADER              1 // Mesh shaders do not work at the moment, do not activate them
#else
    #define BLITZEN_VULKAN_MESH_SHADER              0 
#endif
 

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

    // This is the way Vulkan image resoureces are represented by the Blitzen VulkanRenderer
    struct AllocatedImage
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;

        VkExtent3D extent;
        VkFormat format;

        VmaAllocation allocation;

        // Manually cleanup the resources of an image
        void CleanupResources(VmaAllocator allocator, VkDevice device);

        // Implemented in vulkanResoures.cpp
        ~AllocatedImage();
    };

    struct AllocatedBuffer
    {
        VkBuffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;

        // Implemented on vulkanResources.cpp
        ~AllocatedBuffer();
    };

    struct alignas(16) DrawCullShaderPushConstant
    {
        uint32_t drawCount;

        uint8_t bOcclusionCulling;
        uint8_t bLOD;

        uint8_t bPostPass;

        inline DrawCullShaderPushConstant(uint32_t dc, uint8_t bPP, uint8_t bOC = 1, uint8_t bLod = 1)
        :drawCount{dc}, bPostPass{bPP}, bOcclusionCulling{bOC}, bLOD{bLod} {}
    };

    // The data needed for Vulkan to draw the frame, passed to draw frame function
    struct DrawContext
    {
        void* pCamera;
        void* pMovingCamera;

        uint32_t drawCount;

        uint8_t bOcclusionCulling;
        uint8_t bLOD;

        inline DrawContext(void* pCam, void* pMC, uint32_t dc, uint8_t bOC = 1, uint8_t bLod = 1) 
        : pCamera(pCam), pMovingCamera(pMovingCamera), drawCount(dc), bOcclusionCulling{bOC}, bLOD{bLod} {}
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
    uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator);
}