#pragma once

/*#define VK_NO_PROTOTYPES
#include <Volk/volk.h>*/

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include "Engine/blitzenEngine.h"
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

// Deactivate validation layers on debug mode even if they are requested
#ifdef NDEBUG
#undef BLIT_VK_VALIDATION_LAYERS
#define VK_CHECK(expr)              expr;
#else
#define VK_CHECK(expr)              BLIT_ASSERT(expr == VK_SUCCESS)                                          
#endif

namespace BlitzenVulkan
{
    // These macros will be used to initalize VkApplicationInfo which will be passed to VkInstanceCreateInfo
    constexpr const char* ce_userApp = "Blitzen Game";
    constexpr uint32_t ce_appVersion = VK_MAKE_VERSION (1, 0, 0);
    constexpr const char* ce_hostEngine =  "Blitzen Engine";                             
    constexpr uint32_t ce_userEngineVersion = VK_MAKE_VERSION (BlitzenEngine::ce_blitzenMajor, 0, 0);

    #ifdef BLIT_VSYNC
        constexpr VkPresentModeKHR ce_desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    #else
        constexpr VkPresentModeKHR ce_desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    #endif

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

    constexpr uint32_t ce_maxRequestedInstanceExtensions = 3;

    #ifdef BLIT_VK_RAYTRACING
        constexpr uint8_t ce_bRaytracing = 1;
    #else
        constexpr uint8_t ce_bRaytracing = 0;
    #endif

    constexpr uint32_t ce_maxRequestedDeviceExtensions = 6;

    #ifdef BLIT_DOUBLE_BUFFERING
        constexpr uint8_t ce_framesInFlight = 2;
    #else
        constexpr uint8_t ce_framesInFlight = 1;
    #endif

    #ifdef  BLIT_VK_MESH_EXT
        constexpr uint8_t ce_bMeshShaders = 1;
    #else
        constexpr uint8_t ce_bMeshShaders = 0; 
    #endif

    constexpr VkFormat ce_colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    constexpr VkImageUsageFlags ce_colorAttachmentImageUsage = 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
        VK_IMAGE_USAGE_SAMPLED_BIT | // For generate present compute shader
        VK_IMAGE_USAGE_STORAGE_BIT; // For basic background compute shader

    constexpr VkFormat ce_depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
    constexpr VkImageUsageFlags ce_depthAttachmentImageUsage = 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | 
        VK_IMAGE_USAGE_SAMPLED_BIT; // For generate debug pyramid compute shader




    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;

        uint8_t meshShaderSupport = 0;

        uint8_t bRayTracingSupported = 0;

        uint32_t deviceExtensionCount = 0;
        const char* deviceExtensionNames[ce_maxRequestedDeviceExtensions];

        // Tells some independent function like texture loaders, if their resources are ready to be allocated
        uint8_t bResourceManagementReady = 0;
    };




    struct SurfaceKHR
    {
        VkSurfaceKHR handle = VK_NULL_HANDLE;

        ~SurfaceKHR();
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

        // I have this here if I were want to handle this type of thing like this, but just saying handle is probably better
        inline VkAccelerationStructureKHR get() {return handle;}
    };




    // Because resources get destroyed automatically, when the Vulkan Renderer destructor is called, 
    // It needs to be made certain that the instance, the device and -most importanlty- the allocator are destroyed last
    // So one instance of this struct will be owned by the engine's memory allocator
    struct MemoryCrucialHandles
    {
        VmaAllocator allocator;
        VkDevice device;
        VkInstance instance;

        inline ~MemoryCrucialHandles(){
            vmaDestroyAllocator(allocator);
            vkDestroyDevice(device, nullptr);
            vkDestroyInstance(instance, nullptr);
        }
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

    struct ImageSampler
    {
        VkSampler handle = VK_NULL_HANDLE;

        ~ImageSampler();
    };

    // This will be used to momentarily hold all the textures while loading and then pass them to the descriptor all at once
    struct TextureData
    {
        AllocatedImage image;
        VkSampler sampler;
    };

    // Represents a buffer allocated by VMA
    struct AllocatedBuffer
    {
        VkBuffer bufferHandle = VK_NULL_HANDLE;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;

        // Implemented on vulkanResources.cpp
        ~AllocatedBuffer();
    };

    // Holds a buffer that is bound to a descriptor binding using push descriptors
    template<typename T>
    struct PushDescriptorBuffer
    {
        AllocatedBuffer buffer;

        // Most buffers have a VkWriteDescriptor struct that remains static at runtime
        VkDescriptorBufferInfo bufferInfo{};
        VkWriteDescriptorSet descriptorWrite{};

        // Set these up so that they are defined on the constructor and not hardcoded for every descriptor struct
        uint32_t descriptorBinding;
        VkDescriptorType descriptorType;

        // Persistently mapped pointer, useful for uniform buffers
        T* pData;

        // The constructor expects the binding and the type to be known at initialization
        inline PushDescriptorBuffer(uint32_t binding, VkDescriptorType type)
            : descriptorBinding{binding}, descriptorType{type} {}
    };




    // Holds the command struct for a call to vkCmdDrawIndexedIndirectCount, as well as a draw Id to access the correct RenderObject
    struct IndirectDrawData
    {
        uint32_t drawId;
        VkDrawIndexedIndirectCommand drawIndirect;// 5 32bit integers
    };

    // Holds the command struct for a call to vkCmdDrawMeshTasksIndirectCountExt, as well as a task Id to access the correct task
    struct IndirectTaskData
    {
        uint32_t taskId;
        VkDrawMeshTasksIndirectCommandEXT drawIndirectTasks;// 3 32bit integers
    };

    // Culling shaders receive this shader as a push constant
    struct alignas(16) DrawCullShaderPushConstant
    {
        // Draw count should be checked at the start of the culling shader, 
        // so that the compute shader does not index an out of bounds element in the render object array 
        uint32_t drawCount;

        // Tells the late culling compute shader that it should now check transparent primitives
        uint8_t bPostPass;

        // Constructor
        inline DrawCullShaderPushConstant(uint32_t dc, uint8_t bPP)
        :drawCount{dc}, bPostPass{bPP} {}
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
}