#pragma once

/*#define VK_NO_PROTOTYPES
#include <Volk/volk.h>*/
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "Core/blitzenContainerLibrary.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Renderer/blitRenderingResources.h"
#include "Game/blitObject.h"
#include "Game/blitCamera.h"

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
#define BLITZEN_VULKAN_MESH_SHADER              0 // For now mesh shaders are completely busted 

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
        BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices;

        BlitCL::DynamicArray<uint32_t>& indices;

        BlitCL::DynamicArray<BlitzenEngine::Meshlet>& meshlets;

        BlitCL::DynamicArray<uint32_t>& meshletData;

        BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& transforms;

        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces;

        BlitzenEngine::GameObject* pGameObjects;// This will be transformed to render objects
        size_t gameObjectCount;

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

    // Passed to the renderer every time draw frame is called, to handle special events and update shader data
    struct RenderContext
    {
        uint8_t windowResize = 0;

        /* The renderer assumes that the camera includes the following values:
        BlitML::mat4 projectionMatrix
        BlitML::mat4 viewMatrix
        BlitML::mat4 projectionView
        BlitML::vec3 viewPosition;
        BlitML::mat4 projectionTranspose
        float zNear
        float drawDistance */
        BlitzenEngine::Camera& camera;

        // In case where the camera is detatched, this points to a different camera then the above
        BlitzenEngine::Camera* pDetatchedCamera;

        size_t drawCount;

        BlitML::vec3 sunlightDirection;
        BlitML::vec4 sunlightColor;

        uint8_t debugPyramid = 0;
        uint8_t occlusionEnabled = 1;
        uint8_t lodEnabled = 1;

        inline RenderContext(BlitzenEngine::Camera& cam, BlitzenEngine::Camera* pDetatchedCam, size_t dc, 
        BlitML::vec3& sunDir, BlitML::vec4& sunColor, uint8_t resize,
        uint8_t pyramid = 0, uint8_t occlusion = 1, uint8_t lod = 1)
            :camera(cam), pDetatchedCamera(pDetatchedCam), drawCount(dc), sunlightDirection(sunDir), 
            sunlightColor(sunColor), windowResize(resize), debugPyramid(pyramid), occlusionEnabled(occlusion), 
            lodEnabled(lod)
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

    // Holds universal, shader data that will be passed at the beginning of each frame to the shaders as a uniform buffer
    struct alignas(16) GlobalShaderData
    {
        BlitML::mat4 projectionView;
        BlitML::mat4 view;

        BlitML::vec4 sunlightColor;
        BlitML::vec3 sunlightDir;// directional light direction vector

        BlitML::vec3 viewPosition;
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

        uint32_t drawCount;
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