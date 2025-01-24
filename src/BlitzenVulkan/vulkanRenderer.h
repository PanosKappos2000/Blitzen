#pragma once

#include "vulkanData.h"
#include "Renderer/blitDDSTextures.h"

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
        BlitzenEngine::GlobalShaderData* pGlobalShaderData;

        AllocatedBuffer bufferDeviceAddrsBuffer;
        // Persistently mapped pointer to the uniform buffer for buffer addresses. Dereferenced and updated each frame
        BufferDeviceAddresses* pBufferAddrs;

        AllocatedBuffer cullingDataBuffer;
        BlitzenEngine::CullingData* pCullingData;
    };

    // Holds data for buffers that will be loaded once and will be used for every object
    struct StaticBuffers
    {
        AllocatedBuffer vertexBuffer;

        AllocatedBuffer indexBuffer;

        AllocatedBuffer meshletBuffer;

        AllocatedBuffer meshletDataBuffer;

        AllocatedBuffer globalMaterialBuffer;

        // Array of per object data (StaticRenderObject, got to change that variable name). 1 element for every object
        AllocatedBuffer renderObjectBuffer;

        AllocatedBuffer surfaceBuffer;

        AllocatedBuffer transformBuffer;

        // Holds all the command for draw indirect to draw everything on a scene
        AllocatedBuffer indirectDrawBuffer;

        AllocatedBuffer indirectTaskBuffer;

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

        inline void Cleanup(VmaAllocator allocator, VkDevice device){
            vmaDestroyBuffer(allocator, renderObjectBuffer.buffer, renderObjectBuffer.allocation);
            vmaDestroyBuffer(allocator, globalMaterialBuffer.buffer, globalMaterialBuffer.allocation);

            vmaDestroyBuffer(allocator, meshletBuffer.buffer, meshletBuffer.allocation);
            vmaDestroyBuffer(allocator, meshletDataBuffer.buffer, meshletDataBuffer.allocation);

            vmaDestroyBuffer(allocator, transformBuffer.buffer, transformBuffer.allocation);
            vmaDestroyBuffer(allocator, surfaceBuffer.buffer, surfaceBuffer.allocation);

            vmaDestroyBuffer(allocator, indirectDrawBuffer.buffer, indirectDrawBuffer.allocation);
            vmaDestroyBuffer(allocator, indirectTaskBuffer.buffer, indirectTaskBuffer.allocation);
            vmaDestroyBuffer(allocator, drawIndirectCountBuffer.buffer, drawIndirectCountBuffer.allocation);
            vmaDestroyBuffer(allocator, drawVisibilityBuffer.buffer, drawVisibilityBuffer.allocation);

            vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.allocation);
            vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);

            // Destroy the texture image descriptor set resources
            vkDestroyDescriptorPool(device, textureDescriptorPool, nullptr);
            vkDestroyDescriptorSetLayout(device, textureDescriptorSetlayout, nullptr);
        }
    };

    class VulkanRenderer
    {
    public:

        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight);

        /* ---------------------------------------------------------------------------------------------------------
            2nd part of Vulkan initialization. Setsup buffers, shaders and descriptors for the render loop
        ------------------------------------------------------------------------------------------------------------ */
        void SetupForRendering(GPUData& gpuData, BlitzenEngine::CullingData& cullData);

        void UploadTexture(BlitzenEngine::TextureStats& newTexture);

        // Called each frame to draw the scene that is requested by the engine
        void DrawFrame(BlitzenEngine::RenderContext& pRenderData);

        // Kills the renderer and cleans up allocated handles and resources. Implemented on vulkanInit.cpp
        void Shutdown();

    private:

        void FrameToolsInit();

        void VarBuffersInit();

        void CreateDescriptorLayouts();

        void UploadDataToGPU(BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices, 
        BlitzenEngine::RenderObject* pRenderObjects, size_t renderObjectCount, BlitzenEngine::Material* pMaterials, size_t materialCount, 
        BlitCL::DynamicArray<BlitzenEngine::Meshlet>& meshlets, BlitCL::DynamicArray<uint32_t>& meshletData,
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces, BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& transforms);

        void SetupMainGraphicsPipeline();

        void RecreateSwapchain(uint32_t windowWidth, uint32_t windowHeight);

    public:

        // Array of structs that represent the way textures will be pushed to the GPU
        BlitCL::DynamicArray<TextureData> loadedTextures;

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

        /*
            Descriptor set layout for uniform buffers used by multiple shaders. 
            This includes general data, buffer addresses, culling data etc.
            Use pushes descriptors.
            #binding [0]: global shader data used by both compute and graphics pipelines
            #binding [1]: buffer addresses used by both compute and graphics pipelines
            #binding [3]: culling data used by culling compute shaders
            #binding [4]: depth pyramid combined image sampler used by culling compute shaders for occlusion culling
        */
        VkDescriptorSetLayout m_pushDescriptorBufferLayout;
        /*
            Descriptor set layout for depth pyramid construction. 
            Used by the depth reduce compute pipeline. 
            Uses push descriptors
            # binding[0]: storage image for output image
            # binding[1]: combined image sampler for input image
        */
        VkDescriptorSetLayout m_depthPyramidDescriptorLayout;

        // Pipeline used to draw opaque objects
        VkPipeline m_opaqueGraphicsPipeline;
        VkPipelineLayout m_opaqueGraphicsPipelineLayout;

        VkPipeline m_indirectCullingComputePipeline;

        VkPipeline m_depthReduceComputePipeline;
        VkPipelineLayout m_depthReducePipelineLayout;

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




    void CreateInstance(VkInstance& instance);

    void CreateDebugMessenger(InitializationHandles& m_initHandles);

    uint8_t PickPhysicalDevice(InitializationHandles& initHandles, Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, 
    VulkanStats& stats);

    void CreateDevice(VkDevice& device, InitializationHandles& initHandles, Queue& graphicsQueue, 
    Queue& presentQueue, Queue& computeQueue, VulkanStats& stats);
    
    /*Initializes the swapchain handle that is passed in the newSwapchain argument
    Makes the correct tests to create it according to what the device allows
    oldSwapchain can be passed if the swapchain needs to be recreated*/
    void CreateSwapchain(VkDevice device, InitializationHandles& initHandles, uint32_t windowWidth, uint32_t windowHeight, 
    Queue graphicsQueue, Queue presentQueue, Queue computeQueue, VkAllocationCallbacks* pCustomAllocator, VkSwapchainKHR& newSwapchain, 
    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);

    // Creates the depth pyramid image and mip levels and their data. Needed for occlusion culling
    void CreateDepthPyramid(AllocatedImage& depthPyramidImage, VkExtent2D& depthPyramidExtent, 
    VkImageView* depthPyramidMips, uint8_t& depthPyramidMipLevels, VkSampler& depthAttachmentSampler, 
    VkExtent2D drawExtent, VkDevice device, VmaAllocator allocator, uint8_t createSampler = 1);




    /* ----------------------
        Vulkan Resources 
    ------------------------- */

    // Allocates a buffer using VMA
    void CreateBuffer(VmaAllocator allocator, AllocatedBuffer& buffer, VkBufferUsageFlags bufferUsage, 
    VmaMemoryUsage memoryUsage, VkDeviceSize bufferSize, VmaAllocationCreateFlags allocationFlags);

    // Returns the GPU address of a buffer
    VkDeviceAddress GetBufferAddress(VkDevice device, VkBuffer buffer);

    // Allocates an image resourece using VMA. It also creates a default image view for it
    void CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, 
    VkFormat format, VkImageUsageFlags usage, uint8_t mipLevels = 1);

    // Separate image view creation structure. This is useful for something like the depth pyramid where image views need to be created separately
    void CreateImageView(VkDevice device, VkImageView& imageView, VkImage image, VkFormat format, uint8_t baseMipLevel, uint8_t mipLevels);

    // Allocate an image resource to be used specifically as texture. 
    // The 1st parameter should be the loaded image data that should be passed to the image resource
    void CreateTextureImage(void* data, VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, 
    VkFormat format, VkImageUsageFlags usage, VkCommandBuffer commandBuffer, VkQueue queue, uint8_t loadMipMaps = 0);

    // Placeholder sampler creation function. Used for the default sampler used by all textures so far. 
    // TODO: Replace this with a general purpose function
    void CreateTextureSampler(VkDevice device, VkSampler& sampler);

    // Returns VkFormat based on DDS input to correctly load a texture image
    VkFormat GetDDSVulkanFormat(const BlitzenEngine::DDS_HEADER& header, const BlitzenEngine::DDS_HEADER_DXT10& header10);

    // Returns a VkSampler used only for depth pyramid creation
    // TODO: Replace this with a general purpose function like the above
    VkSampler CreateSampler(VkDevice device, VkSamplerReductionMode reductionMode);

    // Uses vkCmdBlitImage2 to copy a source image to a destination image. Hardcodes alot of parameters. 
    //Can be improved but this is used rarely for now, so I will leave it as is until I have to
    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, 
    VkImage dstImage, VkImageLayout dstLayout, VkExtent2D srcImageSize, VkExtent2D dstImageSize, 
    VkImageSubresourceLayers& srcImageSL, VkImageSubresourceLayers& dstImageSL, VkFilter filter);

    // Copies parts of one buffer to parts of another, depending on the offsets that are passed
    void CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, 
    VkDeviceSize copySize, VkDeviceSize srcOffset, VkDeviceSize dstOffset);

    // Copies data held by a buffer to an image. Used in texture image creation to hold the texture data in the buffer and then pass it to the image
    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage image, VkImageLayout imageLayout, VkExtent3D extent);

    // Allocates a descriptor set that is passed without using push descriptors
    void AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout* pLayouts, 
    uint32_t descriptorSetCount, VkDescriptorSet* pSets);

    // Creates VkWriteDescriptorSet for a buffer type descriptor set
    void WriteBufferDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorBufferInfo& bufferInfo, 
    VkDescriptorType descriptorType, VkDescriptorSet dstSet, uint32_t dstBinding, uint32_t descriptorCount, 
    VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

    // Creates VkWriteDescirptorSet for an image type descirptor set. The image info struct(s) need to be initialized outside
    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo* pImageInfos, VkDescriptorType descriptorType, VkDescriptorSet dstSet, 
    uint32_t descriptorCount, uint32_t binding);

    // Creates VkDescriptorImageInfo and uses it to create a VkWriteDescriptorSet for images. DescriptorCount is set to 1 by default
    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo& imageInfo, VkDescriptorType descirptorType, VkDescriptorSet dstSet, 
    uint32_t binding, VkImageLayout layout, VkImageView imageView, VkSampler sampler = VK_NULL_HANDLE);



    // Puts command buffer in the ready state. vkCmd type function can be called after this and until vkEndCommandBuffer is called
    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags);

    // Submits the command buffer to excecute the recorded commands. Semaphores, fences and other sync structures can be specified 
    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, 
    uint8_t waitSemaphoreCount = 0, VkSemaphore waitSemaphore = VK_NULL_HANDLE, 
    VkPipelineStageFlags2 waitPipelineStage = VK_PIPELINE_STAGE_2_NONE, 
    uint8_t signalSemaphoreCount = 0, VkSemaphore signalSemaphore = VK_NULL_HANDLE, 
    VkPipelineStageFlags2 signalPipelineStage = VK_PIPELINE_STAGE_2_NONE, VkFence fence = VK_NULL_HANDLE);


    
    // Records a command for a pipeline barrier with the specified memory, buffer and image barriers
    void PipelineBarrier(VkCommandBuffer commandBuffer, uint32_t memoryBarrierCount, VkMemoryBarrier2* pMemoryBarriers, uint32_t bufferBarrierCount, 
    VkBufferMemoryBarrier2* pBufferBarriers, uint32_t imageBarrierCount, VkImageMemoryBarrier2* pImageBarriers);

    // Sets up an image memory barrier to be passed to the above function
    void ImageMemoryBarrier(VkImage image, VkImageMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, 
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkImageLayout oldLayout, VkImageLayout newLayout, 
    VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer = 0, 
    uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS);

    // Sets up a buffer memory barrier to be passed to the PipelineBarrier function
    void BufferMemoryBarrier(VkBuffer buffer, VkBufferMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, 
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkDeviceSize offset, VkDeviceSize size);

    // Sets up a memory barrier to be passed to the PipelineBarrier function
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



    
    // Defined in vulkanRenderer.cpp, create a rending attachment info needed to call vkCmdBeginRendering (dynamic rendering)
    void CreateRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, VkImageView imageView, 
    VkImageLayout imageLayout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, 
    VkClearColorValue clearValueColor = {0, 0, 0, 0}, VkClearDepthStencilValue clearValueDepth = {0, 0});

    // Starts a render pass using the dynamic rendering feature(command buffer should be in recording state)
    void BeginRendering(VkCommandBuffer commandBuffer, VkExtent2D renderAreaExtent, VkOffset2D renderAreaOffset, 
    uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* pColorAttachments, VkRenderingAttachmentInfo* pDepthAttachment, 
    VkRenderingAttachmentInfo* pStencilAttachment, uint32_t viewMask = 0, uint32_t layerCount = 1 );

    // Calles the code needed to dynamically set the viewport and scissor. 
    // The graphics pipeline uses a dynamic viewport and scissor by default, so this needs to be called during the frame loop
    void DefineViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent);
}