#pragma once

#include "vulkanData.h"

namespace BlitzenVulkan
{
    struct InitializationHandles
    {
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;

        VkSurfaceKHR surface;

        VkPhysicalDevice chosenGpu;

        VkSwapchainKHR swapchain;
        VkExtent2D swapchainExtent;
        VkFormat swapchainFormat;
        BlitCL::DynamicArray<VkImage> swapchainImages;
        BlitCL::DynamicArray<VkImageView> swapchainImageViews;
    };

    struct Queue
    {
        uint32_t index;
        VkQueue handle;
        uint8_t hasIndex = 0;
    };

    // This struct holds any vulkan structure (buffers, sync structures etc), that need to have an instance for each frame in flight
    struct FrameTools
    {
        VkCommandPool mainCommandPool;
        VkCommandBuffer commandBuffer;

        VkFence inFlightFence;
        VkSemaphore imageAcquiredSemaphore;
        VkSemaphore readyToPresentSemaphore;
    };

    class VulkanRenderer
    {
    public:
        void Init(void* pPlatformState, uint32_t windowWidth, uint32_t windowHeight);

        /* ---------------------------------------------------------------------------------------------------------
            2nd part of Vulkan initialization. Gives scene data in arrays and vulkan uploads the data to the GPU
        ------------------------------------------------------------------------------------------------------------ */
        void UploadDataToGPUAndSetupForRendering();

        /*-----------------------------------------------------------------------------------------------
            Renders the world each frame. 
            If blitzen ever supports other graphics APIs, 
            they will have the same drawFrame function and will define their own render data structs
        -------------------------------------------------------------------------------------------------*/
        void DrawFrame(RenderContext& pRenderData);

        // Kills the renderer and cleans up allocated handles and resources
        void Shutdown();

    private:

        void FrameToolsInit();

        void RecreateSwapchain(uint32_t windowWidth, uint32_t windowHeight);

    private:

        VkDevice m_device;

        VmaAllocator m_allocator;

        VkAllocationCallbacks* m_pCustomAllocator = nullptr;

        InitializationHandles m_initHandles;

        Queue m_graphicsQueue;
        Queue m_presentQueue;
        Queue m_computeQueue;

        VkCommandPool m_placeholderCommandPool;
        VkCommandBuffer m_placeholderCommands;

        AllocatedImage m_colorAttachment;
        AllocatedImage m_depthAttachment;
        VkExtent2D m_drawExtent;

        // This holds tools that need to be unique for each frame in flight
        FrameTools m_frameToolsList[BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT];

        size_t m_currentFrame = 0;

        // Holds stats that give information about how the vulkanRenderer is operating
        VulkanStats m_stats;
    };




    /* --------------------------------------------
        Initialization stage 1 helper functions
    -----------------------------------------------*/

    void CreateSwapchain(VkDevice device, InitializationHandles& initHandles, uint32_t windowWidth, uint32_t windowHeight, 
    Queue graphicsQueue, Queue presentQueue, Queue computeQueue, VkAllocationCallbacks* pCustomAllocator);




    /* ----------------------
        Vulkan Resources 
    ------------------------- */

    void CreateBuffer(VmaAllocator allocator, AllocatedBuffer& buffer, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, VkDeviceSize bufferSize, 
    VmaAllocationCreateFlags allocationFlags);

    void CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags, 
    uint8_t loadMipmaps = 0);

    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, 
    VkExtent2D srcImageSize, VkExtent2D dstImageSize, VkImageSubresourceLayers& srcImageSL, VkImageSubresourceLayers& dstImageSL);




    /*--------------------------------------
        Command Buffer helper functions
    ---------------------------------------*/

    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags);

    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkPipelineStageFlags2 waitPipelineStage,
    VkSemaphore signalSemaphore, VkPipelineStageFlags2 signalPipelineStage, VkFence fence = VK_NULL_HANDLE);


    /*--------------------------------------------------------------
        Pipeline barriers (implemented in vulkanResources.cpp)
    --------------------------------------------------------------*/

    void PipelineBarrier(VkCommandBuffer commandBuffer, uint32_t memoryBarrierCount, VkMemoryBarrier2* pMemoryBarriers, uint32_t bufferBarrierCount, 
    VkBufferMemoryBarrier2* pBufferBarriers, uint32_t imageBarrierCount, VkImageMemoryBarrier2* pImageBarriers);

    void ImageMemoryBarrier(VkImage image, VkImageMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, 
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkImageLayout oldLayout, VkImageLayout newLayout, 
    VkImageSubresourceRange& imageSR);


    /*-------------------------------
        Vulkan Pipelines 
    ---------------------------------*/

    //Helper function that compiles spir-v and adds it to shader stage. Will aid in the creation of graphics and compute pipelines
    void CreateShaderProgram(const VkDevice& device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkShaderModule& shaderModule, VkPipelineShaderStageCreateInfo& pipelineShaderStage, 
    /* I will keep this in case I need to go back to the classic way of doing it */ BlitCL::DynamicArray<char>* shaderCode);

    VkPipelineInputAssemblyStateCreateInfo SetTriangleListInputAssembly();

    void SetDynamicStateViewport(VkDynamicState* pStates, VkPipelineViewportStateCreateInfo& viewportState);

    void SetRasterizationState(VkPipelineRasterizationStateCreateInfo& rasterization, VkPolygonMode polygonMode, VkCullModeFlags cullMode, 
    VkFrontFace frontFace, VkBool32 depthClampEnable = VK_FALSE, VkBool32 depthBiasEnable = VK_FALSE);

    void SetupMulitsampling(VkPipelineMultisampleStateCreateInfo& multisampling, VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples, 
    float minSampleShading, VkSampleMask* pSampleMask, VkBool32 alphaToCoverageEnable, VkBool32 alphaToOneEnable);

    void SetupDepthTest(VkPipelineDepthStencilStateCreateInfo& depthState, VkBool32 depthTestEnable, VkCompareOp depthCompareOp, VkBool32 depthWriteEnable, 
    VkBool32 depthBoundsTestEnable, float maxDepthBounds, float minDepthBounds, VkBool32 stencilTestEnable, VkStencilOpState front, VkStencilOpState back);

    void CreateColorBlendAttachment(VkPipelineColorBlendAttachmentState& colorBlendAttachment, VkColorComponentFlags colorWriteMask, VkBool32 blendEnable, 
    VkBlendOp colorBlendOp, VkBlendOp alphaBlendOp, VkBlendFactor dstAlphaBlendFactor, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstColorBlendFactor, 
    VkBlendFactor srcColorBlendFactor);

    void CreateColorBlendState(VkPipelineColorBlendStateCreateInfo& colorBlending, uint32_t attachmentCount, VkPipelineColorBlendAttachmentState* pAttachments, 
    VkBool32 logicOpEnable, VkLogicOp logicOp);

    //Helper function that creates a pipeline layout that will be used by a compute or graphics pipeline
    void CreatePipelineLayout(VkDevice device, VkPipelineLayout* layout, uint32_t descriptorSetLayoutCount, 
    VkDescriptorSetLayout* pDescriptorSetLayouts, uint32_t pushConstantRangeCount, VkPushConstantRange* pPushConstantRanges);

    //Helper function for pipeline layout creation, takes care of a single set layout binding creation
    void CreateDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding& bindingInfo, uint32_t binding, uint32_t descriptorCount, 
    VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, VkSampler* pImmutableSamplers = nullptr);

    //Helper function for pipeline layout creation, takes care of a single descriptor set layout creation
    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, uint32_t bindingCount, VkDescriptorSetLayoutBinding* pBindings);

    //Helper function for pipeline layout creation, takes care of a single push constant creation
    void CreatePushConstantRange(VkPushConstantRange& pushConstant, VkShaderStageFlags shaderStage, uint32_t size, uint32_t offset = 0);
}