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

namespace BlitzenVulkan
{
    // INCOMPLETE
    inline const char* VK_TRANS_RES(VkResult res)
    {
        switch (res)
        {
        case VK_SUCCESS: return "VULKAN_RES_SUCCESS";
        case VK_NOT_READY: return "VULKAN_RES_NOT_READY";
        case VK_TIMEOUT: return "VULKAN_RES_TIMEDOUT";
        case VK_EVENT_SET: return "VULKAN_RES_EVENT_SET";
        case VK_EVENT_RESET: return "VULKAN_RES_EVENT_RESET";
        case VK_INCOMPLETE: return "VULKAN_RES_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VULKAN_RES_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VULKAN_RES_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VULKAN_RES_INIT_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VULKAN_RES_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VULKAN_RES_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VULKAN_RES_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VULKAN_RES_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VULKAN_RES_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VULKAN_RES_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VULKAN_RES_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VULKAN_RES_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "VULKAN_RES_FRAGMENTED_POOL";
        case VK_RESULT_MAX_ENUM: case VK_ERROR_UNKNOWN: default: return "VULKAN_RES_UNKNOWN";
        }
    }

    inline void VK_RES_MSG_ASSRT(VkResult res)
    {
        BLIT_ASSERT_MESSAGE(res == VK_SUCCESS, VK_TRANS_RES(res));
    }

    inline uint8_t VK_LOG_ERROR_MSG_AND_RETURN(VkResult res)
    {
        if (res < 0)
        {
            BLIT_ERROR("VKRESULT WITH: %s", VK_TRANS_RES(res));
            return 0;
        }

        BLIT_WARN("No error message found");
        return 0;
    }

    constexpr uint32_t Ce_VkApiVersion = VK_API_VERSION_1_3;

    #if defined(BLIT_VK_VALIDATION_LAYERS) && !defined(NDEBUG)

        constexpr uint8_t ce_bValidationLayersRequested = 1;
        constexpr uint8_t Ce_GPUPrintfRequested = 1;

        #if defined(BLIT_VK_SYNCHRONIZATION_VALIDATION)

            constexpr uint8_t Ce_SyncValidationRequested = 1;
        #else

            constexpr uint8_t Ce_SyncValidationRequested = 0;

        #endif
    #else
        constexpr uint8_t ce_bValidationLayersRequested = 0;
        constexpr uint8_t Ce_GPUPrintfRequested = 0;
        constexpr uint8_t Ce_SyncValidationRequested = 0;
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

    #if defined(BLIT_RAYTRACING)
        constexpr uint8_t Ce_RayTracingRequested = 1;
    #else
        constexpr uint8_t Ce_RayTracingRequested = 0;
    #endif

    #if defined(BLIT_MESH_SHADERS)
        constexpr uint8_t Ce_MeshShadersRequested = 1;
    #else
        constexpr uint8_t Ce_MeshShadersRequested = 0;
    #endif

    #if defined(DASHER_JOIN) && defined(DASHER_USE_DEAR)

        constexpr uint8_t Ce_DynamicRenderingExtensionRequested = 1;

    #else

        constexpr uint8_t Ce_DynamicRenderingExtensionRequested = 1;

    #endif

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

    constexpr uint32_t Ce_PresentWaitMaxCount = 2;

    constexpr VkImageUsageFlags Ce_SwapchainImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    constexpr uint32_t Ce_SwapchainDescriptorBinding = 0;
    constexpr uint32_t Ce_MaxSwapchainImageCount = 16;
    constexpr VkFormat Ce_DesiredSwapchainSurfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
    constexpr VkColorSpaceKHR Ce_DesiredSwapchainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    // The format and usage flags that will be set for the color and depth attachments
    constexpr VkFormat Ce_ColorTargetFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    constexpr VkImageLayout Ce_ColorTargetLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    constexpr VkImageUsageFlags Ce_ColorTargetUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    constexpr uint32_t Ce_ColorTargetDescriptorBinding = 1;
    constexpr VkClearColorValue ce_WindowClearColor =
    {
        BlitzenCore::Ce_DefaultWindowBackgroundColor[0],
        BlitzenCore::Ce_DefaultWindowBackgroundColor[1],
        BlitzenCore::Ce_DefaultWindowBackgroundColor[2],
        BlitzenCore::Ce_DefaultWindowBackgroundColor[3]
    };

    constexpr VkFormat Ce_DepthTargetFormat = VK_FORMAT_D32_SFLOAT;
    constexpr VkImageLayout Ce_DepthTargetLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    constexpr VkImageUsageFlags Ce_DepthTargetUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr uint32_t Ce_DepthTargetDescriptorBinding = 1;
    constexpr uint32_t Ce_DepthTargetDescriptorID = 0;

    constexpr VkFormat Ce_DepthPyramidFormat = VK_FORMAT_R32_SFLOAT;
    constexpr VkImageUsageFlags Ce_DepthPyramidImageUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    constexpr uint8_t ce_maxDepthPyramidMipLevels = 16;
    constexpr uint32_t Ce_HI_Z_MAPDescriptorID = 1;

    // The size of the stack arrays that hold push descriptor writes
    #if defined(BLITZEN_CLUSTER_CULLING)
        constexpr uint32_t Ce_ComputeDescriptorWriteArraySize = 8;
    #else
        constexpr uint32_t Ce_ComputeDescriptorWriteArraySize = 7;
    #endif

    
    constexpr uint32_t Ce_PushDescriptorSetID = 0;
    
    // GRAPHICS DESCRIPTORS
    constexpr uint32_t Ce_GraphicsDescriptorCount = 2;

    constexpr uint32_t Ce_VertexBufferDescriptorBinding = 1;
    constexpr uint32_t Ce_VertexBufferGraphicsPushID = 0;

    constexpr uint32_t Ce_MatBufferDescriptorBinding = 6;
    constexpr uint32_t Ce_MatBufferGraphicsPushID = 1;

    constexpr uint32_t Ce_TextureDescriptorsBinding = 0;
    constexpr uint32_t Ce_TextureDescriptorsSetID = 1;

    // SHARED DESCRIPTORS
    constexpr uint32_t Ce_SharedDescriptorCount = 4;

    constexpr uint32_t Ce_ViewDataBufferDescriptorBinding = 0;
    constexpr uint32_t Ce_ViewDataBufferSharedPushID = 0;

    constexpr uint32_t Ce_SurfaceBufferDescriptorBinding = 2;
    constexpr uint32_t Ce_SurfaceBufferSharedPushID = 1;

    constexpr uint32_t Ce_TransformBufferDescriptorBinding = 5;
    constexpr uint32_t Ce_TransformBufferSharedPushID = 2;

    constexpr uint32_t Ce_DrawCmdBufferDescriptorBinding = 7;
    constexpr uint32_t Ce_DrawCmdBufferSharedPushID = 3;

    // CULL DESCRIPTORS
    constexpr uint32_t Ce_CullDescriptorCount = 2;

    constexpr uint32_t Ce_LODBufferDescriptorBinding = 4;
    constexpr uint32_t Ce_LODBufferCullPushID = 0;

    constexpr uint32_t Ce_DrawCmdCounterDescriptorBinding = 9;
    constexpr uint32_t Ce_DrawCmdCounterCullPushID = 1;

    // DRAW OCC DESCRIPTORS
    constexpr uint32_t Ce_DrawOcclusionDescriptorCount = 1;

    constexpr uint32_t Ce_DrawVisBufferDescriptorBinding = 10;
    constexpr uint32_t Ce_DrawVisBufferOccPushID = 0;

    // HI_Z Culling Descriptor
    constexpr uint32_t Ce_HI_Z_CullBinding = 3;

    // Cluster culling descriptors
    constexpr uint32_t Ce_ClusterCullDescriptorCount = 1;

    constexpr uint32_t Ce_ClusterBufferDescriptorBinding = 12;
    constexpr uint32_t Ce_ClusterBufferPushID = 0;

    // Tlas 
    constexpr uint32_t Ce_TlasBufferBinding = 13;

    // HI_Z GENERATION DESCRIPTORS
    constexpr uint32_t Ce_HI_Z_DescriptorCount = 2;

    constexpr uint32_t Ce_HI_Z_DstImageBinding = 0;
    constexpr uint32_t Ce_HI_Z_SrcImageBinding = 1;

    constexpr uint32_t Ce_DefaultPushDescriptorBindingCount = 10;

    constexpr size_t ce_textureStagingBufferSize = 128 * 1024 * 1024;

    constexpr uint64_t ce_fenceTimeout = 1000000000;
    constexpr uint64_t ce_swapchainImageTimeout = ce_fenceTimeout;



    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;

        uint8_t meshShaderSupport = 0;

        uint8_t bSynchronizationValidationSupported = 0;

        uint8_t bRayTracingSupported = 0;

        BlitCL::DynamicArray<const char*> m_instExtensions;
        BlitCL::DynamicArray<const char*> m_dvExtensions;
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
        VkSwapchainKHR m_handle;

        VkExtent2D m_extent;

        VkFormat m_format;

        VkImage m_images[Ce_MaxSwapchainImageCount];
        VkImageView m_views[Ce_MaxSwapchainImageCount];
        uint32_t m_imageCount{ 0 };

        uint32_t m_minImageCount{ 0 };

        ~Swapchain();
    };

    struct Buffer
    {
        VkBuffer m_handle{ VK_NULL_HANDLE };

        VmaAllocation m_vmaAlloc;
        VmaAllocationInfo m_vmaInfo;

        ~Buffer();
    };

    struct Image
    {
        VkImage m_handle{ VK_NULL_HANDLE };
        VmaAllocation m_vmaAlloc;

        ~Image();
    };

    struct ImageView
    {
        VkImageView m_handle{ VK_NULL_HANDLE };

        ~ImageView();
    };

    struct ImageSampler
    {
        VkSampler m_handle{ VK_NULL_HANDLE };

        ~ImageSampler();
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

    struct BlitVk_SSBO
    {
        Buffer m_buffer;
    };

    template<class DATA>
    struct BlitVk_CPU_DATA_SSBO
    {
        Buffer m_buffer;

        Buffer m_staging;

        DATA* m_pMapped{ nullptr };

        size_t m_copyDataSize { 0 };
    };

    template<class DATA>
    struct BlitVk_UBUFFER
    {
        Buffer m_buffer;

        DATA* m_pMapped{ nullptr };
    };

    struct BlitVk_2DIMAGE
    {
        Image m_image;
        ImageView m_view;

        uint32_t m_width;
        uint32_t m_height;
    };

    struct BlitVk_2DIMAGE_SAMP
    {
        BlitVk_2DIMAGE m_image;

        ImageSampler m_samp;
    };

    struct HI_Z_MAP
    {
        BlitVk_2DIMAGE m_pyramid;

        VkImageView m_levels[ce_maxDepthPyramidMipLevels];
        uint8_t m_levelCount;

        ~HI_Z_MAP();
    };

    struct TextureData
    {
        BlitVk_2DIMAGE image;
        VkSampler sampler;
    };



    /*
        Vulkan specific shader data structs
    */
    
    struct IndirectDrawData
    {
        uint32_t drawId;
        VkDrawIndexedIndirectCommand drawIndirect;// 5 32bit integers
    };
    constexpr uint32_t Ce_DrawCmdElementCount = 500'000;

    struct IndirectTaskData
    {
        uint32_t taskId;
        VkDrawMeshTasksIndirectCommandEXT drawIndirectTasks;// 3 32bit integers
    };

    // TODO: Either remove lodIndex or add padding in the future
    struct ClusterGroupData
    {
        uint32_t objectId;
        uint32_t lodIndex;
        uint32_t clusterId;

        uint32_t padding0;
    };
    static_assert(sizeof(ClusterGroupData) % 16 == 0, "Unexpected alignment for ClusterGroupData");
    constexpr uint32_t Ce_ClusterGroupBufferSize = 1'000'000;
    constexpr uint32_t Ce_TransClusterGouprBufferSize = 10'000;

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

// Deactivate validation layers on debug mode even if they are requested
#if defined(NDEBUG)

#define VK_CHECK(expr)              expr;

#define VK_CHECK_MSG(expr)          expr;

#else

#define VK_CHECK(expr)              BLIT_ASSERT(expr == VK_SUCCESS)

#define VK_CHECK_MSG(expr)               BlitzenVulkan::VK_RES_MSG_ASSRT(expr)

#endif


namespace BlitzenPlatform
{
    // Creates the surface used by the vulkan renderer. Implemented on Platform.cpp
    uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator, void* pPlatform);

    void UNSET_VALIDATION_LAYER_LENS();
}