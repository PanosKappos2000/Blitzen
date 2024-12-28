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

    struct VarBuffers
    {
        AllocatedBuffer globalShaderDataBuffer;
        // Persistently mapped pointer to the uniform buffer for shader data. Dereferenced and updated each frame
        GlobalShaderData* pGlobalShaderData;

        AllocatedBuffer bufferDeviceAddrsBuffer;
        // Persistently mapped pointer to the uniform buffer for buffer addresses. Dereferenced and updated each frame
        BufferDeviceAddresses* pBufferAddrs;

        AllocatedBuffer cullingDataBuffer;
        CullingData* pCullingData;
    };

    // Holds data for buffers that will be loaded once and will be used for every object
    struct StaticBuffers
    {
        AllocatedBuffer globalVertexBuffer;

        AllocatedBuffer globalIndexBuffer;

        AllocatedBuffer globalMeshBuffer;

        AllocatedBuffer globalMaterialBuffer;

        // Array of per object data (StaticRenderObject, got to change that variable name). 1 element for every object
        AllocatedBuffer renderObjectBuffer;

        AllocatedBuffer drawIndirectBuffer;

        // Holds all the command for draw indirect to draw everything on a scene
        AllocatedBuffer drawIndirectBufferFinal;

        // Counts how many objects have actually been added to the final draw indirect buffer(helps avoid empty draw calls)
        AllocatedBuffer drawIndirectCountBuffer;

        // Holds an array of integers with an element for each object. The integer is 0 or 1, depending on if the associated object was visible last frame
        AllocatedBuffer drawVisibilityBuffer;

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

            vmaDestroyBuffer(allocator, drawIndirectBuffer.buffer, drawIndirectBuffer.allocation);
            vmaDestroyBuffer(allocator, drawIndirectBufferFinal.buffer, drawIndirectBufferFinal.allocation);
            vmaDestroyBuffer(allocator, drawIndirectCountBuffer.buffer, drawIndirectCountBuffer.allocation);
            vmaDestroyBuffer(allocator, drawVisibilityBuffer.buffer, drawVisibilityBuffer.allocation);

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

        void VarBuffersInit();

        void UploadDataToGPU(BlitCL::DynamicArray<BlitML::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices, 
        BlitCL::DynamicArray<StaticRenderObject>& staticObjects, BlitCL::DynamicArray<MaterialConstants>& materials, 
        BlitCL::DynamicArray<BlitML::Meshlet>& meshlets, BlitCL::DynamicArray<IndirectOffsets>& indirectDraws);

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
        VkSampler m_depthAttachmentSampler;// This is needed for depth pyramid and occlusion tests

        AllocatedImage m_depthPyramid;
        VkImageView m_depthPyramidMips[16];
        uint8_t m_depthPyramidMipLevels = 0;
        VkExtent2D m_depthPyramidExtent;

        StaticBuffers m_currentStaticBuffers;

        // Structures needed to pass the global shader data
        VkDescriptorSetLayout m_globalShaderDataLayout;
        GlobalShaderData m_globalShaderData;

        // Pipeline used to draw opaque objects
        VkPipeline m_opaqueGraphicsPipeline;
        VkPipelineLayout m_opaqueGraphicsPipelineLayout;// Right now this layout is also used for the culling compute pipeline but I might want to change that

        VkPipeline m_indirectCullingComputePipeline;

        VkPipeline m_depthReduceComputePipeline;
        VkPipelineLayout m_depthReducePipelineLayout;
        VkDescriptorSetLayout m_depthPyramidImageDescriptorSetLayout;

        VkPipeline m_lateCullingComputePipeline;// This one is for the shader that does visibility tests on objects that were rejected last frame
        VkPipelineLayout m_lateCullingPipelineLayout;

        // This holds tools that need to be unique for each frame in flight
        FrameTools m_frameToolsList[BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT];

        VarBuffers m_varBuffers[BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT];

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

    VkDeviceAddress GetBufferAddress(VkDevice device, VkBuffer buffer);

    void CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, 
    uint8_t mipLevels = 1);

    void CreateImageView(VkDevice device, VkImageView& imageView, VkImage image, VkFormat format, uint8_t baseMipLevel, uint8_t mipLevels);

    void CreateTextureImage(void* data, VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, 
    VkCommandBuffer commandBuffer, VkQueue queue, uint8_t loadMipMaps = 0);

    void CreateTextureSampler(VkDevice device, VkSampler& sampler);

    VkSampler CreateSampler(VkDevice device, VkSamplerReductionMode reductionMode);

    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, 
    VkExtent2D srcImageSize, VkExtent2D dstImageSize, VkImageSubresourceLayers& srcImageSL, VkImageSubresourceLayers& dstImageSL, VkFilter filter);

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
    VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer = 0, 
    uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS);

    void BufferMemoryBarrier(VkBuffer buffer, VkBufferMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, 
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkDeviceSize offset, VkDeviceSize size);

    void MemoryBarrier(VkMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, 
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage);


    /*-------------------------------
        Vulkan Pipelines 
    ---------------------------------*/

    /*
        This function reads a spir-v shader in byte format from the filepath passed in the second parameter.
        It needs a reference to an empty shader module(so that it does not go out of scope beofre pipeline creation).
        It also needs an empty shader stage create info, that will be filled by the function and shoulder later be passed to the pipeline info
        The shader stage and entry point parameters specify the type of shader(eg. compute) and the main function name respectively

        The finally dynamic array parameter will probably be removed shortly(TODO)
    */
    void CreateShaderProgram(const VkDevice& device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkShaderModule& shaderModule, VkPipelineShaderStageCreateInfo& pipelineShaderStage);

    // Since the creation of a compute pipeline is very simple, the function can be a wrapper around CreateShaderProgram with a bit of extra code
    void CreateComputeShaderProgram(VkDevice device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkPipelineLayout& layout, VkPipeline* pPipeline);

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
    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, uint32_t bindingCount, VkDescriptorSetLayoutBinding* pBindings, 
    VkDescriptorSetLayoutCreateFlags flags = 0);

    //Helper function for pipeline layout creation, takes care of a single push constant creation
    void CreatePushConstantRange(VkPushConstantRange& pushConstant, VkShaderStageFlags shaderStage, uint32_t size, uint32_t offset = 0);



    /*----------------------------
        Other helper functions
    -----------------------------*/

    // Defined in vulkanRenderer.cpp
    void CreateRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, VkImageView imageView, VkImageLayout imageLayout, 
    VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearColorValue clearValueColor = {0, 0, 0, 0}, VkClearDepthStencilValue clearValueDepth = {0, 0});

    // Starts a render pass using the dynamic rendering feature(command buffer should be in recording state)
    void BeginRendering(VkCommandBuffer commandBuffer, VkExtent2D renderAreaExtent, VkOffset2D renderAreaOffset, 
    uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* pColorAttachments, VkRenderingAttachmentInfo* pDepthAttachment, 
    VkRenderingAttachmentInfo* pStencilAttachment, uint32_t viewMask = 0, uint32_t layerCount = 1 );

    void DefineViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent);
}