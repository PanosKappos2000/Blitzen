#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "Engine/blitzenEngine.h"
#include "BlitCL/DynamicArray.h"
#include "BlitCL/blitArray.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Renderer/blitRenderingResources.h"

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
	// VkApplicationInfo
    constexpr const char* ce_userApp = "Blitzen Game";
    constexpr uint32_t ce_appVersion = VK_MAKE_VERSION (1, 0, 0);
    constexpr const char* ce_hostEngine =  "Blitzen Engine";                             
    constexpr uint32_t ce_userEngineVersion = VK_MAKE_VERSION (BlitzenEngine::ce_blitzenMajor, 0, 0);

	// Swapchain
    #ifdef BLIT_VSYNC
        constexpr VkPresentModeKHR ce_desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    #else
        constexpr VkPresentModeKHR ce_desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    #endif

    // Validation layer definitions
    #if defined(BLIT_VK_VALIDATION_LAYERS) && !defined(NDEBUG)
        constexpr uint8_t ce_bValidationLayersRequested = 1;
        #if defined(BLIT_VK_SYNCHRONIZATION_VALIDATION)
            constexpr uint8_t ce_bSynchronizationValidationRequested = 1;
        #else
            constexpr uint8_t ce_bSynchronizationValidationRequested = 0;
        #endif
    #else
        constexpr uint8_t ce_bValidationLayersRequested = 0;
        constexpr uint8_t ce_bSynchronizationValidationRequested = 0;
    #endif

	// Total extensions count
    constexpr uint32_t ce_maxRequestedInstanceExtensions = 3;
    constexpr uint32_t ce_maxRequestedDeviceExtensions = 8;

	// Is raytracing requested?
    #if defined(BLIT_VK_RAYTRACING)
        constexpr uint8_t ce_bRaytracing = 1;
    #else
        constexpr uint8_t ce_bRaytracing = 0;
    #endif
    
    // Double buffering 
    #if defined(BLIT_DOUBLE_BUFFERING)
        constexpr uint8_t ce_framesInFlight = 2;
    #else
        constexpr uint8_t ce_framesInFlight = 1;
    #endif

    // Are mesh shaders requested?
    #if defined(BLIT_VK_MESH_EXT)
        constexpr uint8_t ce_bMeshShaders = 1;
    #else
        constexpr uint8_t ce_bMeshShaders = 0; 
    #endif

    // The format and usage flags that will be set for the color and depth attachments
    constexpr VkFormat ce_colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    constexpr VkClearColorValue ce_WindowClearColor = { 0.f, 0.2f, 0.4f, 1.f };
    constexpr VkImageLayout ce_ColorAttachmentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    constexpr VkImageUsageFlags ce_colorAttachmentImageUsage = 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
        VK_IMAGE_USAGE_SAMPLED_BIT | // For generate present compute shader
        VK_IMAGE_USAGE_STORAGE_BIT; // For basic background compute shader

    constexpr VkFormat ce_depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    constexpr VkImageLayout ce_DepthAttachmentLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    constexpr VkImageUsageFlags ce_depthAttachmentImageUsage = 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | 
        VK_IMAGE_USAGE_SAMPLED_BIT; // For generate debug pyramid compute shader
    constexpr uint8_t ce_maxDepthPyramidMipLevels = 16;
    
    // Indices into the push descriptor writes array
    constexpr uint32_t ce_viewDataWriteElement = 0;
    constexpr uint32_t Ce_DepthPyramidImageBindingID = 3;

    // The size of the stack arrays that hold push descriptor writes
    #if defined(BLITZEN_CLUSTER_CULLING)
        constexpr uint32_t Ce_ComputeDescriptorWriteArraySize = 9;
        constexpr uint32_t Ce_GraphicsDescriptorWriteArraySize = 8;
    #else
        constexpr uint32_t Ce_ComputeDescriptorWriteArraySize = 8;
        constexpr uint32_t Ce_GraphicsDescriptorWriteArraySize = 8;
    #endif

    // The allocation size of the indirect draw buffer and all buffers that depend on it
    const uint32_t IndirectDrawElementCount = 10'000'000;

	// When passing a pointer to a vulkan API function, with only one element(common occurence),
    // prefer passing this instead of one for clarity
    constexpr uint32_t Ce_SinglePointer = 1;


    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;

        uint8_t meshShaderSupport = 0;

        uint8_t bSynchronizationValidationSupported = 0;

        uint8_t bRayTracingSupported = 0;

        uint32_t deviceExtensionCount = 0;
        const char* deviceExtensionNames[ce_maxRequestedDeviceExtensions];

        uint8_t bResourceManagementReady = 0;

        uint8_t bObliqueNearPlaneClippingObjectsExist = 0;

        uint8_t bTranspartentObjectsExist = 0;
    };




    /*
        RAII wappers for Vulkan handles
    */
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




    // TODO: There are better way to offset the functionality of this struct
    struct MemoryCrucialHandles
    {
        VmaAllocator allocator;
        VkDevice device;
        VkInstance instance;

		static MemoryCrucialHandles* GetInstance()
		{
            return s_pInstance;
		}

        inline MemoryCrucialHandles()
        {
            s_pInstance = this;
        }

        inline ~MemoryCrucialHandles()
        {
            vmaDestroyAllocator(allocator);
            vkDestroyDevice(device, nullptr);
            vkDestroyInstance(instance, nullptr);
        }

    private:

        inline static MemoryCrucialHandles* s_pInstance;
    };



    
    /*
        Vulkan resources structs (image, buffers)
    */
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
    // TODO: Test alignment
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

    // TODO: This is std430, TEST ALIGNMENT!!!
    struct ClusterDispatchData
    {
        uint32_t objectId;
        uint32_t lodIndex;
        uint32_t clusterId;
    };

	struct ClusterCullShaderPushConstant
	{
        VkDeviceAddress renderObjectBufferAddress;
        VkDeviceAddress clusterDispatchBufferAddress;
        VkDeviceAddress clusterCountBufferAddress;
        uint32_t drawCount;
	};

    struct alignas(16) DrawCullShaderPushConstant
    {
        VkDeviceAddress renderObjectBufferDeviceAddress;
        uint32_t drawCount;
    };

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
    uint8_t CreateVulkanSurface(VkInstance& instance, VkSurfaceKHR& surface, VkAllocationCallbacks* pAllocator);

    // Gets the memory crucial handles form the memory manager (defined in blitzenMemory.cpp)
    BlitzenVulkan::MemoryCrucialHandles* GetVulkanMemoryCrucials();

}