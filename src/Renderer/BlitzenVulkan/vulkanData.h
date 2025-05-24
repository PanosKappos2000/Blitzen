#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "Core/blitzenEngine.h"
#include "BlitCL/DynamicArray.h"
#include "BlitCL/blitArray.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Renderer/Resources/blitRenderingResources.h"

// My math library seems to be fine now but I am keeping this to compare values when needed
/*#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"*/

// Deactivate validation layers on debug mode even if they are requested
#if defined(NDEBUG)
#undef BLIT_VK_VALIDATION_LAYERS
#define VK_CHECK(expr)              expr;
#else
#define VK_CHECK(expr)              BLIT_ASSERT(expr == VK_SUCCESS)                                          
#endif

namespace BlitzenVulkan
{
    constexpr uint32_t Ce_VkApiVersion = VK_API_VERSION_1_3;


    constexpr uint32_t Ce_MaxRequestedInstanceExtensions = 3;

    constexpr uint8_t Ce_SurfaceExtensionsRequired = 1;

    constexpr uint8_t Ce_ValidationExtensionRequired = 0;
    constexpr uint32_t Ce_ValidationExtensionElement = 2;

    #if defined(BLIT_VK_VALIDATION_LAYERS) && !defined(NDEBUG)
        constexpr uint8_t ce_bValidationLayersRequested = 1;
        constexpr uint8_t Ce_GPUPrintfDeviceExtensionRequested = 1;
        #if defined(BLIT_VK_SYNCHRONIZATION_VALIDATION)
            constexpr uint8_t ce_bSynchronizationValidationRequested = 1;
        #else
            constexpr uint8_t ce_bSynchronizationValidationRequested = 0;
        #endif
    #else
        constexpr uint8_t ce_bValidationLayersRequested = 0;
        constexpr uint8_t Ce_GPUPrintfDeviceExtensionRequested = 0;
        constexpr uint8_t ce_bSynchronizationValidationRequested = 0;
    #endif

    // Platform specific expressions
    #if defined(_WIN32)
        constexpr const char* ce_surfaceExtensionName = "VK_KHR_win32_surface";
        constexpr const char* ce_baseValidationLayerName = "VK_LAYER_KHRONOS_validation";
    #elif linux
        constexpr const char* ce_surfaceExtensionName = "VK_KHR_xcb_surface";
        constexpr const char* ce_baseValidationLayerName = "VK_LAYER_NV_optimus";
        #define VK_USE_PLATFORM_XCB_KHR
    #endif

    constexpr uint32_t Ce_MaxValidationLayerCount = 2;
    constexpr const char* Ce_SyncValidationLayerName = "VK_LAYER_KHRONOS_synchronization2";


	// Total extensions count
    constexpr uint32_t Ce_MaxRequestedDeviceExtensions = 8;

    constexpr uint32_t Ce_SwapchainExtnsionElement = 0;
    constexpr uint8_t Ce_SwapchainExtensionRequested = 1;
    constexpr uint8_t Ce_SwapchainExtensionRequired = 1;

    constexpr uint32_t Ce_PushDescriptorExtensionElement = 1;
    constexpr uint8_t Ce_IsPushDescriptorExtensionsRequested = 1;
    constexpr uint8_t Ce_IsPushDescriptorExtensionsRequired = 1;

    #if defined(BLIT_RAYTRACING)
        constexpr uint8_t Ce_RayTracingRequested = 1;
    #else
        constexpr uint8_t Ce_RayTracingRequested = 0;
    #endif
    constexpr uint8_t Ce_RayTracingRequired = 0;

    constexpr uint32_t Ce_MeshShaderExtensionElement = 5;
    #if defined(BLIT_MESH_SHADERS)
        constexpr uint8_t Ce_MeshShadersRequested = 1;
    #else
        constexpr uint8_t Ce_MeshShadersRequested = 0;
    #endif
        constexpr uint8_t Ce_MeshShadersRequired = 0;

    constexpr uint32_t Ce_SyncValidationDeviceExtensionElement = 6;
    constexpr uint8_t Ce_SyncValidationDeviceExtensionRequired = 0;

    constexpr uint32_t Ce_GPUPrintfDeviceExtensionElement = 7;
    constexpr uint8_t Ce_GPUPrintfDeviceExtensionRequired = 0;


    constexpr uint32_t Ce_MaxUniqueueDeviceQueueIndices = 4;

    constexpr uint32_t Ce_GraphicsQueueInfoIndex = 0;
    constexpr uint32_t Ce_TransferQueueInfoIndex = 1;
    constexpr uint32_t Ce_ComputeQueueInfoIndex = 2;


    // Double buffering 
#if defined(BLIT_DOUBLE_BUFFERING)
    constexpr uint8_t ce_framesInFlight = 2;
#else
    constexpr uint8_t ce_framesInFlight = 1;
#endif

#if defined(BLIT_VSYNC)
    constexpr VkPresentModeKHR Ce_DesiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
#else
    constexpr VkPresentModeKHR Ce_DesiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
#endif

    constexpr VkImageUsageFlags Ce_SwapchainImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // The format and usage flags that will be set for the color and depth attachments
    constexpr VkFormat ce_colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    constexpr VkImageLayout ce_ColorAttachmentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    constexpr VkImageUsageFlags ce_colorAttachmentImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    constexpr VkClearColorValue ce_WindowClearColor =
    {
        BlitzenCore::Ce_DefaultWindowBackgroundColor[0],
        BlitzenCore::Ce_DefaultWindowBackgroundColor[1],
        BlitzenCore::Ce_DefaultWindowBackgroundColor[2],
        BlitzenCore::Ce_DefaultWindowBackgroundColor[3]
    };

    constexpr VkFormat ce_depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    constexpr VkImageLayout ce_DepthAttachmentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    constexpr VkImageUsageFlags ce_depthAttachmentImageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    constexpr VkFormat Ce_DepthPyramidFormat = VK_FORMAT_R32_SFLOAT;
    constexpr VkImageUsageFlags Ce_DepthPyramidImageUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    constexpr uint8_t ce_maxDepthPyramidMipLevels = 16;

    // The size of the stack arrays that hold push descriptor writes
    #if defined(BLITZEN_CLUSTER_CULLING)
        constexpr uint32_t Ce_ComputeDescriptorWriteArraySize = 8;
    #else
        constexpr uint32_t Ce_ComputeDescriptorWriteArraySize = 7;
    #endif

    constexpr uint32_t ce_viewDataWriteElement = 0;
    constexpr uint32_t Ce_LodBufferDescriptorId = 1;
    constexpr uint32_t Ce_TransformBufferDrawCullDescriptorId = 2;
    constexpr uint32_t Ce_DrawCmdBufferDrawCullDescriptorId = 3;
    constexpr uint32_t Ce_DrawCountBufferDrawCullDescriptorId = 4;
    constexpr uint32_t Ce_VisibilityBufferDrawCullDescriptorId = 5;
    constexpr uint32_t Ce_SurfaceBufferDrawCullDescriptorId = 6;
    constexpr uint32_t Ce_ClusterBufferDrawCullDescriptorId = 7;

    constexpr uint32_t Ce_DepthPyramidImageBindingID = 3;

    constexpr uint32_t Ce_GraphicsDescriptorWriteArraySize = 6;

    constexpr uint32_t Ce_VertexBufferPushDescriptorId = 1;
    constexpr uint32_t Ce_MaterialBufferPushDescriptorId = 2;
    constexpr uint32_t Ce_TransformBufferGraphicsDescriptorId = 3;
    constexpr uint32_t Ce_DrawCmdBufferGraphicsDescriptorId = 4;
    constexpr uint32_t Ce_SurfaceBufferGraphicsDescriptorId = 5;

    constexpr uint32_t Ce_StaticSSBODataCount = 10;

    constexpr uint32_t Ce_VertexBufferDataCopyIndex = 0;
    constexpr uint32_t Ce_IndexBufferDataCopyIndex = 1;
    constexpr uint32_t Ce_OpaqueRenderBufferCopyIndex = 2;
    constexpr uint32_t Ce_TransparentRenderBufferCopyIndex = 3;
    constexpr uint32_t Ce_ONPCRenderBufferCopyIndex = 4;
    constexpr uint32_t Ce_SurfaceBufferDataCopyIndex = 5;
    constexpr uint32_t Ce_LodBufferDataCopyIndex = 6;
    constexpr uint32_t Ce_MaterialBufferDataCopyIndex = 7;
    constexpr uint32_t Ce_ClusterBufferDataCopyIndex = 8;
    constexpr uint32_t Ce_ClusterIndexBufferDataCopyIndex = 9;

    
    constexpr uint32_t IndirectDrawElementCount = 10'000'000;
    constexpr uint32_t Ce_TrasparentDispatchElementCount = 500'000;

	// TODO: REMOVE THIS 
    constexpr uint32_t Ce_SinglePointer = 1;

    constexpr size_t ce_textureStagingBufferSize = 128 * 1024 * 1024;

    constexpr uint32_t PushDescriptorSetID = 0;// Used when calling PuhsDesriptors for the set parameter
    constexpr uint32_t TextureDescriptorSetID = 1;

    constexpr uint8_t Ce_LateCulling = 1;// Sets the late culling boolean to 1
    constexpr uint8_t Ce_InitialCulling = 0;// Sets the late culling boolean to 0

    constexpr uint64_t ce_fenceTimeout = 1000000000;
    constexpr uint64_t ce_swapchainImageTimeout = ce_fenceTimeout;



    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;

        uint8_t meshShaderSupport = 0;

        uint8_t bSynchronizationValidationSupported = 0;

        uint8_t bRayTracingSupported = 0;

        uint32_t deviceExtensionCount = 0;
        const char* deviceExtensionNames[Ce_MaxRequestedDeviceExtensions];

        uint8_t bResourceManagementReady = 0;

        uint8_t bObliqueNearPlaneClippingObjectsExist = 0;

        uint8_t bTranspartentObjectsExist = 0;
    };




    /* RAII wappers for Vulkan handles */
    struct SurfaceKHR
    {
        VkSurfaceKHR handle = VK_NULL_HANDLE;

        ~SurfaceKHR();
    };

    struct Queue
    {
        uint32_t index;
        VkQueue handle;
        uint8_t hasIndex = 0;
    };

    struct PipelineObject
    {   
        VkPipeline handle = VK_NULL_HANDLE;

        ~PipelineObject();
    };

    struct PipelineLayout
    {
        VkPipelineLayout handle = VK_NULL_HANDLE;

        ~PipelineLayout();
    };

    struct ShaderModule
    {
        VkShaderModule handle = VK_NULL_HANDLE;

        ~ShaderModule();
    };

    struct DescriptorSetLayout
    {
        VkDescriptorSetLayout handle = VK_NULL_HANDLE;

        ~DescriptorSetLayout();
    };

    struct DescriptorPool
    {
        VkDescriptorPool handle = VK_NULL_HANDLE;

        ~DescriptorPool();
    };

    struct PipelineProgram
    {
        PipelineObject& pipeline;
        PipelineLayout& layout;

        inline PipelineProgram(PipelineObject& p, PipelineLayout& l) :pipeline(p), layout(l) {}
    };

    struct CommandPool
    {
        VkCommandPool handle = VK_NULL_HANDLE;

        ~CommandPool();
    };

    struct Semaphore
    {
        VkSemaphore handle = VK_NULL_HANDLE;
        
        ~Semaphore();
    };

    struct SyncFence
    {
        VkFence handle = VK_NULL_HANDLE;

        ~SyncFence();
    };

    struct AccelerationStructure
    {
        VkAccelerationStructureKHR handle = VK_NULL_HANDLE;

        ~AccelerationStructure();
    };

    struct Swapchain
    {
        VkSwapchainKHR swapchainHandle;

        VkExtent2D swapchainExtent;

        VkFormat swapchainFormat;

        BlitCL::DynamicArray<VkImage> swapchainImages;

        BlitCL::DynamicArray<VkImageView> swapchainImageViews;

        ~Swapchain();
    };




    /* Vulkan resources structs (image, buffers) */
    struct MemoryCrucialHandles
    {
        VmaAllocator allocator;
        VkDevice device;
        VkInstance instance;

        inline ~MemoryCrucialHandles()
        {
            vmaDestroyAllocator(allocator);
            vkDestroyDevice(device, nullptr);
            vkDestroyInstance(instance, nullptr);
        }
    };

    MemoryCrucialHandles* InitMemoryCrucialHandles(MemoryCrucialHandles* pHandles);


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

    struct ImageSampler
    {
        VkSampler handle = VK_NULL_HANDLE;

        ~ImageSampler();
    };

    struct PushDescriptorImage
    {
        AllocatedImage image;
        
        ImageSampler sampler;

        VkDescriptorType m_descriptorType;
        uint32_t m_descriptorBinding;
        VkImageLayout m_layout;

        VkWriteDescriptorSet descriptorWrite{};
        VkDescriptorImageInfo descriptorInfo{};

        inline PushDescriptorImage(VkDescriptorType type, uint32_t binding, VkImageLayout layout) 
            :m_descriptorType{type}, m_descriptorBinding{binding}, m_layout{layout}
        {}

        uint8_t SamplerInit(VkDevice device, VkFilter filter, VkSamplerMipmapMode mipmapMode, 
            VkSamplerAddressMode addressMode, void* pNextChain);
    };

    struct TextureData
    {
        AllocatedImage image;
        VkSampler sampler;
    };

    struct AllocatedBuffer
    {
        VkBuffer bufferHandle = VK_NULL_HANDLE;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;

        ~AllocatedBuffer();
    };

    template<typename T>
    struct PushDescriptorBuffer
    {
        AllocatedBuffer buffer;

        VkDescriptorBufferInfo bufferInfo{};
        VkWriteDescriptorSet descriptorWrite{};

        uint32_t descriptorBinding;
        VkDescriptorType descriptorType;

        T* pData;

        inline PushDescriptorBuffer(uint32_t binding, VkDescriptorType type)
            : descriptorBinding{binding}, descriptorType{type} {}
    };



    /*
        Vulkan specific shader data structs
    */
    
    struct IndirectDrawData
    {
        uint32_t drawId;
        VkDrawIndexedIndirectCommand drawIndirect;// 5 32bit integers
    };

    struct IndirectTaskData
    {
        uint32_t taskId;
        VkDrawMeshTasksIndirectCommandEXT drawIndirectTasks;// 3 32bit integers
    };

    // TODO: Either remove lodIndex or add padding in the future
    struct ClusterDispatchData
    {
        uint32_t objectId;
        uint32_t lodIndex;
        uint32_t clusterId;
    };

	struct alignas(16) ClusterCullShaderPushConstant
	{
        VkDeviceAddress renderObjectBufferAddress;
        VkDeviceAddress clusterDispatchBufferAddress;
        VkDeviceAddress clusterCountBufferAddress;
        uint32_t drawCount;
        uint32_t padding0;
	};
    static_assert(sizeof(ClusterCullShaderPushConstant) == 32, "Unexpected size for ClusterCullShaderPushConstant");
    static_assert(alignof(ClusterCullShaderPushConstant) == 16, "Unexpected alignment for ClusterCullShaderPushConstant");

    struct alignas(16) DrawCullShaderPushConstant
    {
        VkDeviceAddress renderObjectBufferDeviceAddress;
        uint32_t drawCount;
        uint32_t padding0;
    };
    static_assert(sizeof(DrawCullShaderPushConstant) == 16, "Unexpected size for DrawCullShaderPushConstant");
    static_assert(alignof(DrawCullShaderPushConstant) == 16, "Unexpected alignment for DrawCullShaderPushConstant");

    struct GlobalShaderDataPushConstant
    {
        VkDeviceAddress renderObjectBufferDeviceAddress;
    };

    struct BackgroundShaderPushConstant
    {
        BlitML::vec4 data1;
        BlitML::vec4 data2;
        BlitML::vec4 data3;
        BlitML::vec4 data4;
    };
}


namespace BlitzenPlatform
{
    // Creates the surface used by the vulkan renderer. Implemented on Platform.cpp
    uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator, void* pPlatform);
}