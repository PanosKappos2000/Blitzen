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

        // Any buffers that will be getting their data update every frame, need to be inside the frame tools struct
        // Actually the above is kind of naive, the buffers will be separated into their own struct, so that they can be reloaded separately if necessary
        // TODO: See above
        VkDescriptorPool globalShaderDataDescriptorPool;
        AllocatedBuffer globalShaderDataBuffer;
        AllocatedBuffer bufferDeviceAddrsBuffer;
    };

    // Holds data for buffers that will be loaded once and will be used for every object
    struct StaticBuffers
    {
        AllocatedBuffer globalVertexBuffer;

        AllocatedBuffer globalIndexBuffer;

        AllocatedBuffer globalMeshBuffer;

        AllocatedBuffer globalMaterialBuffer;

        AllocatedBuffer renderObjectBuffer;

        AllocatedBuffer drawIndirectBuffer;

        // Holds the addresses of each one of the above buffers(except global shader data buffer)
        BufferDeviceAddresses bufferAddresses;

        // A single descriptor pool will allocate a big set with one binding to hold all the textures that are loaded
        VkDescriptorPool textureDescriptorPool;
        VkDescriptorSetLayout textureDescriptorSetlayout;
        VkDescriptorSet textureDescriptorSet;

        // Holds all vulkan texture data for each texture loaded to the GPU
        BlitCL::DynamicArray<TextureData> loadedTextures;

        inline void Cleanup(VmaAllocator allocator, VkDevice device){
            vmaDestroyBuffer(allocator, renderObjectBuffer.buffer, renderObjectBuffer.allocation);
            vmaDestroyBuffer(allocator, globalMaterialBuffer.buffer, globalMaterialBuffer.allocation);
            #if BLITZEN_VULKAN_MESH_SHADER
                vmaDestroyBuffer(allocator, globalMeshBuffer.buffer, globalMeshBuffer.allocation);
            #endif
            //vmaDestroyBuffer(allocator, drawIndirectBuffer.buffer, drawIndirectBuffer.allocation);
            vmaDestroyBuffer(allocator, globalIndexBuffer.buffer, globalIndexBuffer.allocation);
            vmaDestroyBuffer(allocator, globalVertexBuffer.buffer, globalVertexBuffer.allocation);

            // Destroy the texture image descriptor set resources
            vkDestroyDescriptorPool(device, textureDescriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(device, textureDescriptorSetlayout, nullptr);
            for(size_t i = 0; i < loadedTextures.GetSize(); ++i)
            {
                loadedTextures[i].image.CleanupResources(allocator, device);
            }
        }
    };

    class VulkanRenderer
    {
    public:

        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight);

        /* ---------------------------------------------------------------------------------------------------------
            2nd part of Vulkan initialization. Gives scene data in arrays and vulkan uploads the data to the GPU
        ------------------------------------------------------------------------------------------------------------ */
        void UploadDataToGPUAndSetupForRendering(GPUData& gpuData);

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

        void UploadDataToGPU(BlitCL::DynamicArray<BlitML::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices, 
        BlitCL::DynamicArray<StaticRenderObject>& staticObjects, BlitCL::DynamicArray<MaterialConstants>& materials, 
        BlitCL::DynamicArray<BlitML::Meshlet>& meshlets = BlitCL::DynamicArray<BlitML::Meshlet>());

        void RecreateSwapchain(uint32_t windowWidth, uint32_t windowHeight);

    private:

        // Handle to the logical device
        VkDevice m_device;

        // Used to allocate vulkan resources like buffers and images
        VmaAllocator m_allocator;

        // Custom allocator, don't need it right now
        VkAllocationCallbacks* m_pCustomAllocator = nullptr;

        // Holds objects crucial to the renderer that are initalized on the 1st init stage but will not be mentioned that often
        InitializationHandles m_initHandles;

        // All the queue handles and indices retrieved from the device on initialization
        Queue m_graphicsQueue;
        Queue m_presentQueue;
        Queue m_computeQueue;

        // Temporary command buffer and its command pool, used before the game loop starts
        VkCommandPool m_placeholderCommandPool;
        VkCommandBuffer m_placeholderCommands;

        // Data for the rendering attachments
        AllocatedImage m_colorAttachment;
        AllocatedImage m_depthAttachment;
        VkExtent2D m_drawExtent;

        StaticBuffers m_currentStaticBuffers;

        // Structures needed to pass the global shader data
        VkDescriptorSetLayout m_globalShaderDataLayout;
        GlobalShaderData m_globalShaderData;

        // Pipeline used to draw opaque objects
        VkPipeline m_opaqueGraphicsPipeline;
        VkPipelineLayout m_opaqueGraphicsPipelineLayout;

        // This holds tools that need to be unique for each frame in flight
        FrameTools m_frameToolsList[BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT];

        // Used to access the right frame tools depending on which ones are already being used
        size_t m_currentFrame = 0;

        // Holds stats that give information about how the vulkanRenderer is operating
        VulkanStats m_stats;

        // I do not need a sampler for each texture and there is a limit for each device, so I'll need to create only a few samlplers
        VkSampler m_placeholderSampler;
    };




    /* --------------------------------------------
        Initialization stage 1 helper functions
    -----------------------------------------------*/

    void CreateSwapchain(VkDevice device, InitializationHandles& initHandles, uint32_t windowWidth, uint32_t windowHeight, 
    Queue graphicsQueue, Queue presentQueue, Queue computeQueue, VkAllocationCallbacks* pCustomAllocator, VkSwapchainKHR& newSwapchain, 
    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);




    /* ----------------------
        Vulkan Resources 
    ------------------------- */

    void CreateBuffer(VmaAllocator allocator, AllocatedBuffer& buffer, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, VkDeviceSize bufferSize, 
    VmaAllocationCreateFlags allocationFlags);

    void CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, 
    uint8_t loadMipmaps = 0);

    void CreateTextureImage(void* data, VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, 
    VkCommandBuffer commandBuffer, VkQueue queue, uint8_t loadMipMaps = 0);

    void CreateTextureSampler(VkDevice device, VkSampler& sampler);

    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, 
    VkExtent2D srcImageSize, VkExtent2D dstImageSize, VkImageSubresourceLayers& srcImageSL, VkImageSubresourceLayers& dstImageSL);

    void CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize copySize, VkDeviceSize srcOffset, 
    VkDeviceSize dstOffset);

    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage image, VkImageLayout imageLayout, VkExtent3D extent);

    void AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout* pLayouts, uint32_t descriptorSetCount, VkDescriptorSet* pSets);
    void WriteBufferDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorBufferInfo& bufferInfo, VkDescriptorType descriptorType, VkDescriptorSet dstSet, 
    uint32_t dstBinding, uint32_t descriptorCount, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo* pImageInfos, VkDescriptorType descriptorType, VkDescriptorSet dstSet, 
    uint32_t descriptorCount, uint32_t binding);




    /*--------------------------------------
        Command Buffer helper functions
    ---------------------------------------*/

    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags);

    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, uint8_t waitSemaphoreCount = 0, 
    VkSemaphore waitSemaphore = VK_NULL_HANDLE, VkPipelineStageFlags2 waitPipelineStage = VK_PIPELINE_STAGE_2_NONE, uint8_t signalSemaphoreCount = 0,
    VkSemaphore signalSemaphore = VK_NULL_HANDLE, VkPipelineStageFlags2 signalPipelineStage = VK_PIPELINE_STAGE_2_NONE, VkFence fence = VK_NULL_HANDLE);


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

    void SetDynamicStateViewport(VkDynamicState* pStates, VkPipelineViewportStateCreateInfo& viewportState, VkPipelineDynamicStateCreateInfo& dynamicState);

    void SetRasterizationState(VkPipelineRasterizationStateCreateInfo& rasterization, VkPolygonMode polygonMode, VkCullModeFlags cullMode, 
    VkFrontFace frontFace, VkBool32 depthClampEnable = VK_FALSE, VkBool32 depthBiasEnable = VK_FALSE);

    void SetupMulitsampling(VkPipelineMultisampleStateCreateInfo& multisampling, VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples, 
    float minSampleShading, VkSampleMask* pSampleMask, VkBool32 alphaToCoverageEnable, VkBool32 alphaToOneEnable);

    void SetupDepthTest(VkPipelineDepthStencilStateCreateInfo& depthState, VkBool32 depthTestEnable, VkCompareOp depthCompareOp, VkBool32 depthWriteEnable, 
    VkBool32 depthBoundsTestEnable, float maxDepthBounds, float minDepthBounds, VkBool32 stencilTestEnable, 
    VkStencilOpState* front, VkStencilOpState* back);

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



    /*----------------------------
        Other helper functions
    -----------------------------*/

    // Defined in vulkanRenderer.cpp
    void CreateRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, VkImageView imageView, VkImageLayout imageLayout, 
    VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearColorValue clearValueColor = {0, 0, 0, 0}, VkClearDepthStencilValue clearValueDepth = {0, 0});
}