#pragma once
#include "vulkanData.h"
#include "Renderer/blitDDSTextures.h"
#include "Game/blitCamera.h"
#include "Game/blitObject.h"

namespace BlitzenVulkan
{

    class VulkanRenderer
    {

    public:

        VulkanRenderer();
        ~VulkanRenderer();
        VulkanRenderer operator = (const VulkanRenderer& vk) = delete;

        // Initalizes the Vulkan API.
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight);

        // Sets up the Vulkan renderer for drawing according to the resources loaded by the engine
        uint8_t SetupForRendering(BlitzenEngine::RenderingResources* pResources, float& pyramidWidth, float& pyramidHeight);

        // Function for DDS texture loading
        uint8_t UploadTexture(void* pData, const char* filepath);

    public:

        // This struct holds any vulkan structure (buffers, sync structures etc), that need to have an instance for each frame in flight
        struct FrameTools
        {
            CommandPool mainCommandPool;
            VkCommandBuffer commandBuffer;

            CommandPool transferCommandPool;
            VkCommandBuffer transferCommandBuffer;

            SyncFence inFlightFence;
            Semaphore imageAcquiredSemaphore;
            Semaphore buffersReadySemaphore;
            Semaphore readyToPresentSemaphore;
        };

        // The renderer will have one instance of these buffers, which will include buffers that my be updated
        struct VarBuffers
        {
            PushDescriptorBuffer<BlitzenEngine::CameraViewData> viewDataBuffer{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };

            PushDescriptorBuffer<void> transformBuffer{ 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            AllocatedBuffer transformStagingBuffer;
            BlitzenEngine::MeshTransform* pTransformData = nullptr;
        };

        // Shows a loading screen while waiting for resources to be loaded
        void DrawWhileWaiting();

        void SetupWhileWaitingForPreviousFrame(const BlitzenEngine::DrawContext& context);

        // Called each frame to draw the scene that is requested by the engine
        void DrawFrame(BlitzenEngine::DrawContext& context);

        // When a dynamic object moves, it should call this function to update the staging buffer
        void UpdateObjectTransform(uint32_t trId, BlitzenEngine::MeshTransform& newTr);

        // Static function that allows access to vulkan renderer at any scope
        inline static VulkanRenderer* GetRendererInstance() { return m_pThisRenderer; }
        inline VulkanStats GetStats() const { return m_stats; }

        // Vulkan API and memory crucials
        VkInstance m_instance;
        VmaAllocator m_allocator;
        VkDevice m_device;

    private:

        // Initalizes structure needed to call the DrawWhileWaiting function
        uint8_t CreateIdleDrawHandles();

        // Creates loading triangle pipeline
        bool CreateLoadingTrianglePipeline();

        // Creates structures that allow Vulkan to allocate resources (command buffers, allocator, texture sampler) 
        void SetupResourceManagement();

        // Initializes the rendering attachments and the depth pyramid
        uint8_t RenderingAttachmentsInit();

        // Takes an opened DDS file's data and uploads the data to the void* parameter
        bool LoadDDSImageData(BlitzenEngine::DDS_HEADER& header,
            BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::FileHandle& handle,
            VkFormat& vulkanImageFormat, void* pData);

        // Initalizes the frame tools array
        uint8_t FrameToolsInit();

        // Creates the descriptor set latyouts that are not constant and need to have one instance for each frame in flight
        uint8_t CreateDescriptorLayouts();

        VkDescriptorSetLayout CreateGPUBufferPushDescriptorBindings(VkDescriptorSetLayoutBinding* pBindings, 
            uint32_t bindingCount);

        uint8_t CreatePipelineLayouts();

        // Creates the buffers that are dynamic and might be updated each frame
        uint8_t VarBuffersInit(BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& transforms);

        // Creates the constant buffers and upload their data
        uint8_t StaticBuffersInit(BlitzenEngine::RenderingResources* pResources);

        // Takes the staging buffers and copies their content to the GPU buffers
        void CopyStaticBufferDataToGPUBuffers(VkBuffer stagingVertexBuffer, VkDeviceSize vertexBufferSize,
            VkBuffer stagingIndexBuffer, VkDeviceSize indexBufferSize,
            VkBuffer renderObjectStagingBuffer, VkDeviceSize renderObjectBufferSize,
            VkBuffer transparentObjectStagingBuffer, VkDeviceSize trasparentRenderBufferSize,
            VkBuffer onpcRenderObjectStagingBuffer, VkDeviceSize onpcRenderObjectBufferSize,
            VkBuffer surfaceStagingBuffer, VkDeviceSize surfaceBufferSize,
            VkBuffer materialStagingBuffer, VkDeviceSize materialBufferSize,
            VkDeviceSize visibilityBufferSize,
            VkBuffer clusterStagingBuffer, VkDeviceSize clusterBufferSize,
            VkBuffer clusterIndicesStagingBuffer, VkDeviceSize clusterIndicesBufferSize);

        void SetupGpuBufferDescriptorWriteArrays();

        // Creates main compute shaders. Implemented in vulkanPipeline.cpp
        uint8_t CreateComputeShaders();

        // Creates most of the graphics pipelines. I need to refactor this
        uint8_t SetupMainGraphicsPipeline();

        uint8_t BuildBlas(const BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces,
            const BlitCL::DynamicArray<uint32_t>& primitiveVertexCounts);

        uint8_t BuildTlas(BlitzenEngine::RenderObject* pDraws, uint32_t drawCount,
            BlitzenEngine::MeshTransform* pTransforms, BlitzenEngine::PrimitiveSurface* pSurface);



        // Updates var buffers
        void UpdateBuffers(BlitzenEngine::RenderingResources* pResources, FrameTools& tools, VarBuffers& buffers);

        // For occlusion culling to be possible a depth pyramid needs to be generated based on the depth attachment
        void GenerateDepthPyramid(VkCommandBuffer commandBuffer);

        void DrawBackgroundImage(VkCommandBuffer commandBuffer);

        // Calls the compute shader that copies the color attachment to the swapchain image
        void CopyColorAttachmentToSwapchainImage(VkCommandBuffer commandBuffer, VkImageView swapchainView, VkImage swapchainImage);

        // Recreates the swapchain when necessary (and other handles that are involved with the window, like the depth pyramid)
        void RecreateSwapchain(uint32_t windowWidth, uint32_t windowHeight);

        /*
            Private helper structs
        */
    private:

        // This struct is pointless, should just have what is inside separately
        struct StaticBuffers
        {
            PushDescriptorBuffer<void> vertexBuffer{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            AllocatedBuffer indexBuffer;

            PushDescriptorBuffer<void> clusterBuffer{ 12, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            PushDescriptorBuffer<void> meshletDataBuffer{ 13, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

            PushDescriptorBuffer<void> materialBuffer{ 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

            // TODO: Turn these into buffers that use Buffer Address
            PushDescriptorBuffer<void> renderObjectBuffer{ 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            VkDeviceAddress renderObjectBufferAddress;
            PushDescriptorBuffer<void> onpcReflectiveRenderObjectBuffer{ 14, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            VkDeviceAddress onpcRenderObjectBufferAddress;
            PushDescriptorBuffer<void> transparentRenderObjectBuffer{ 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            VkDeviceAddress transparentRenderObjectBufferAddress;

            PushDescriptorBuffer<void> surfaceBuffer{ 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

            PushDescriptorBuffer<void> indirectDrawBuffer{ 7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            PushDescriptorBuffer<void> indirectCountBuffer{ 9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

            PushDescriptorBuffer<void> indirectTaskBuffer{ 8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

            PushDescriptorBuffer<void> visibilityBuffer{ 10, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

            PushDescriptorBuffer<void> clusterDispatchBuffer{ 16, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            PushDescriptorBuffer<void> clusterCountBuffer{ 17, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            AllocatedBuffer clusterCountCopyBuffer;

            AllocatedBuffer blasBuffer;
            BlitCL::DynamicArray<AccelerationStructure> blasData;
            PushDescriptorBuffer<void> tlasBuffer{ 15, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR };
            AccelerationStructure tlasData;
        };

        /*
            API initialization handles section
        */
    private:

        VkAllocationCallbacks* m_pCustomAllocator;

        VkDebugUtilsMessengerEXT m_debugMessenger;

        // WARNING: Do not move swapchain declaration above this
        // Destructor order is important for these 2 handles
        SurfaceKHR m_surface;

        VkPhysicalDevice m_physicalDevice;

        Swapchain m_swapchainValues;

        /*
            Image resources section
        */
    private:

        // Size of color attachments (right now it's always the same size as the swapchain)
        VkExtent2D m_drawExtent;

        // Color attachment. Its sampler will be used to copy it into the swapchain
        PushDescriptorImage m_colorAttachment
        {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1, // binding ID
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkRenderingAttachmentInfo m_colorAttachmentInfo{};

        // Depth attachment. Its sampler will be passed to the depth pyramid generation shader, binding 1
        PushDescriptorImage m_depthAttachment
        {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1, // bidning ID
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkRenderingAttachmentInfo m_depthAttachmentInfo{};

        // The depth pyramid will copy data from the depth attachment to create a pyramid for occlusion culling
        PushDescriptorImage m_depthPyramid
        {
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            0, // binding ID
            VK_IMAGE_LAYOUT_GENERAL
        };
        VkImageView m_depthPyramidMips[ce_maxDepthPyramidMipLevels];
        uint8_t m_depthPyramidMipLevels;
        VkExtent2D m_depthPyramidExtent;

        // Will hold all textures that will be loaded for the scene, to pass them to the global descriptor set later
        TextureData loadedTextures[BlitzenEngine::ce_maxTextureCount];
        size_t textureCount;
        ImageSampler m_textureSampler;

        /*
            Buffer resources section
        */
    private:

        StaticBuffers m_currentStaticBuffers;

        VarBuffers m_varBuffers[ce_framesInFlight];

        /*
            Descriptor section
        */
    private:

        // Layout for descriptors that will be using PushDescriptor extension. Has 10+ bindings
        DescriptorSetLayout m_pushDescriptorBufferLayout;

        BlitCL::StaticArray<VkWriteDescriptorSet, Ce_GraphicsDescriptorWriteArraySize> 
            pushDescriptorWritesGraphics;
        VkWriteDescriptorSet pushDescriptorWritesCompute[Ce_ComputeDescriptorWriteArraySize];

        // Layout for descriptor set that passes the source image and dst image for each depth pyramid mip
        DescriptorSetLayout m_depthPyramidDescriptorLayout;

        // Descriptor layout for the texture descriptors. 1 binding that holds an array of textures
        DescriptorSetLayout m_textureDescriptorSetlayout;

        // This descriptor set does not use push descriptors and thus it needs to be allocated with a descriptor pool
        DescriptorPool m_textureDescriptorPool;
        VkDescriptorSet m_textureDescriptorSet;

        // Descriptor set layout for the backup shader, used when draw count is 0
        DescriptorSetLayout m_backgroundImageSetLayout;

        // Descriptor set layout for src and dst image for the generate presentation compute shader
        DescriptorSetLayout m_generatePresentationImageSetLayout;

        /*
            Pipelines section
        */
    private:

        // Main graphics pipeline. Draws opaque objects that have no special properties
        PipelineObject m_opaqueGeometryPipeline;
        PipelineObject m_postPassGeometryPipeline;
        PipelineLayout m_graphicsPipelineLayout;

        // Draws objects that use the near plane clipping matrix.
        PipelineObject m_onpcReflectiveGeometryPipeline;
        PipelineLayout m_onpcReflectiveGeometryLayout;

        // Oblique Near place clipping culling pipeline. Might not be necessary
        PipelineObject m_onpcDrawCullPipeline;

        // Draws a triangle. Used while resources are being loaded, so that the screen in not white. Temporary
        PipelineObject m_loadingTrianglePipeline;
        PipelineLayout m_loadingTriangleLayout;
        BlitML::vec3 m_loadingTriangleVertexColor;

        // Culling shaders. Initial does furstum culling and LOD selection
        // Late culling exists to add occlusion culling
        PipelineObject m_initialDrawCullPipeline;
        PipelineObject m_lateDrawCullPipeline;
        PipelineObject m_transparentDrawCullPipeline;
        PipelineLayout m_drawCullLayout;

        PipelineObject m_intialClusterCullPipeline;

        // The depth pyramid generation pipeline will hold a helper compute shader for the late culling pipeline.
        // It will generate the depth pyramid from the 1st pass' depth buffer. It will then be used for occlusion culling 
        PipelineObject m_depthPyramidGenerationPipeline;
        PipelineLayout m_depthPyramidGenerationLayout;

        // Used when draw count is 0 to generate a default background (might use it with normal drawing as well in the future)
        PipelineObject m_basicBackgroundPipeline;
        PipelineLayout m_basicBackgroundLayout;

        // Copies the color attachment to the current swapchain image. Used at the end of the DrawFrame function
        PipelineObject m_generatePresentationPipeline;
        PipelineLayout m_generatePresentationLayout;

        /*
            Runtime section
        */
    private:

        FrameTools m_frameToolsList[ce_framesInFlight];

        // Used for any loading pipeline
        CommandPool m_idleCommandBufferPool;
        VkCommandBuffer m_idleDrawCommandBuffer;

        // Frame tools index
        uint8_t m_currentFrame;

        // Holds stats that give information about how the vulkanRenderer is operating
        VulkanStats m_stats;

        Queue m_graphicsQueue;
        Queue m_presentQueue;
        Queue m_computeQueue;
        Queue m_transferQueue;

        // Static singleton pointer
        static VulkanRenderer* m_pThisRenderer;
    };




    // Creates the swapchain
    uint8_t CreateSwapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice,
        uint32_t windowWidth, uint32_t windowHeight,
        Queue graphicsQueue, Queue presentQueue, Queue computeQueue,
        VkAllocationCallbacks* pCustomAllocator,
        Swapchain& newSwapchain, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);

    // Creates the depth pyramid image and mip levels and their data. Needed for occlusion culling
    uint8_t CreateDepthPyramid(PushDescriptorImage& depthPyramidImage, VkExtent2D& depthPyramidExtent,
        VkImageView* depthPyramidMips, uint8_t& depthPyramidMipLevels,
        VkExtent2D drawExtent, VkDevice device, VmaAllocator allocator);


    // Creates VkWriteDescriptorSet for a buffer type descriptor set
    void WriteBufferDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorBufferInfo& bufferInfo,
        VkDescriptorType descriptorType, uint32_t dstBinding, VkBuffer buffer, void* pNextChain = nullptr,
        VkDescriptorSet dstSet = VK_NULL_HANDLE, VkDeviceSize offset = 0, uint32_t descriptorCount = 1,
        VkDeviceSize range = VK_WHOLE_SIZE, uint32_t dstArrayElement = 0);

    // Creates VkWriteDescirptorSet for an image type descirptor set. The image info struct(s) need to be initialized outside
    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo* pImageInfos, 
        VkDescriptorType descriptorType, VkDescriptorSet dstSet,
        uint32_t descriptorCount, uint32_t binding, uint32_t dstArrayElement = 0);

    // Creates VkDescriptorImageInfo and uses it to create a VkWriteDescriptorSet for images. DescriptorCount is set to 1 by default
    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo& imageInfo,
        VkDescriptorType descirptorType, VkDescriptorSet dstSet,
        uint32_t binding, VkImageLayout layout, VkImageView imageView, VkSampler sampler = VK_NULL_HANDLE);



    // Puts command buffer in the ready state. vkCmd type function can be called after this and until vkEndCommandBuffer is called
    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags);

    void DispatchRenderObjectCullingComputeShader(VkCommandBuffer commandBuffer,
        VkPipeline pipeline, VkPipelineLayout layout,
        uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites,
        VkBuffer indirectCountBuffer, VkBuffer indirectCommandsBuffer, VkBuffer visibilityBuffer,
        PushDescriptorImage& depthAttachment, PushDescriptorImage& depthPyramid,
        uint32_t drawCount, VkDeviceAddress renderObjectBufferAddress,
        uint8_t lateCulling, VkInstance instance);

    void DispatchPreClusterCullingShader(VkCommandBuffer, 
        VkPipeline pipeline, VkPipelineLayout layout, 
        uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites, 
        VkBuffer clusterCountBuffer, VkBuffer clusterDataBuffer, VkBuffer clusterCountCopy,
        uint32_t drawCount, VkDeviceAddress renderObjectBufferAddress, 
        uint8_t lateCulling, VkInstance instance);

    void DispatchClusterCullingComputeShader(VkCommandBuffer, 
        VkPipeline pipeline, VkPipelineLayout layout,
        uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites,
        VkBuffer clusterCountBuffer, AllocatedBuffer& clusterCountCopyBuffer, VkBuffer clusterDispatchBuffer,
        VkBuffer drawCountBuffer, VkBuffer indirectDrawBuffer, 
        uint32_t drawCount, VkDeviceAddress renderObjectBufferAddress,
        VkInstance instance);

    // Handles draw calls using draw indirect commands that should already be set by culling compute shaders
    void DrawGeometry(VkCommandBuffer commandBuffer,
        VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorWriteCount,
        VkPipeline pipeline, VkPipelineLayout layout, VkDescriptorSet* textureSet,
        VkRenderingAttachmentInfo& colorAttachmentInfo, VkRenderingAttachmentInfo& depthAttachmentInfo,
        VkExtent2D drawExtent,
        VkBuffer indirectDrawBuffer, VkBuffer indirectCountBuffer, VkBuffer indexBuffer,
        uint32_t drawCount, VkDeviceAddress renderObjectBufferAddress,
        uint8_t latePass, VkInstance instance,
        uint8_t bRaytracing, uint32_t tlasCount, VkAccelerationStructureKHR* pTlas,
        uint8_t onpcPass = 0, BlitML::mat4* pOnpcMatrix = nullptr);

    // Copies parts of one buffer to parts of another, depending on the offsets that are passed
    void CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
        VkDeviceSize copySize, VkDeviceSize srcOffset, VkDeviceSize dstOffset);

    // Defined in vulkanRenderer.cpp, create a rending attachment info needed to call vkCmdBeginRendering (dynamic rendering)
    void CreateRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, VkImageView imageView,
        VkImageLayout imageLayout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
        VkClearColorValue clearValueColor = { 0, 0, 0, 0 }, VkClearDepthStencilValue clearValueDepth = { 0, 0 });

    // Starts a render pass using the dynamic rendering feature(command buffer should be in recording state)
    void BeginRendering(VkCommandBuffer commandBuffer, VkExtent2D renderAreaExtent, VkOffset2D renderAreaOffset,
        uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* pColorAttachments, VkRenderingAttachmentInfo* pDepthAttachment,
        VkRenderingAttachmentInfo* pStencilAttachment, uint32_t viewMask = 0, uint32_t layerCount = 1);

    // Calles the code needed to dynamically set the viewport and scissor. 
    // The graphics pipeline uses a dynamic viewport and scissor by default, so this needs to be called during the frame loop
    void DefineViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent);

    void CreateSemahoreSubmitInfo(VkSemaphoreSubmitInfo& semaphoreInfo,
        VkSemaphore semaphore, VkPipelineStageFlags2 stage);

    // Submits the command buffer to excecute the recorded commands. Semaphores, fences and other sync structures can be specified 
    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, uint32_t waitSemaphoreCount = 0,
        VkSemaphoreSubmitInfo* pWaitInfo = nullptr, uint32_t signalSemaphoreCount = 0,
        VkSemaphoreSubmitInfo* signalSemaphore = nullptr, VkFence fence = VK_NULL_HANDLE);

    void PresentToSwapchain(VkDevice device, VkQueue queue,
        VkSwapchainKHR* pSwapchains, uint32_t swapchainCount,
        uint32_t waitSemaphoreCount, VkSemaphore* pWaitSemaphores,
        uint32_t* pImageIndices, VkResult* pResults = nullptr, void* pNextChain = nullptr);

    // Records a command for a pipeline barrier with the specified memory, buffer and image barriers
    void PipelineBarrier(VkCommandBuffer commandBuffer, uint32_t memoryBarrierCount,
        VkMemoryBarrier2* pMemoryBarriers, uint32_t bufferBarrierCount,
        VkBufferMemoryBarrier2* pBufferBarriers, uint32_t imageBarrierCount,
        VkImageMemoryBarrier2* pImageBarriers);

    // Sets up an image memory barrier to be passed to the above function
    void ImageMemoryBarrier(VkImage image, VkImageMemoryBarrier2& barrier,
        VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage,
        VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage,
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount,
        uint32_t baseArrayLayer = 0, uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS);

    // Sets up a buffer memory barrier to be passed to the PipelineBarrier function
    void BufferMemoryBarrier(VkBuffer buffer, VkBufferMemoryBarrier2& barrier,
        VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage,
        VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage,
        VkDeviceSize offset, VkDeviceSize size);

    // Sets up a memory barrier to be passed to the PipelineBarrier function
    void MemoryBarrier(VkMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage,
        VkAccessFlags2 firstAccessStage, VkPipelineStageFlags2 secondSyncStage,
        VkAccessFlags2 secondAccessStage);



    /*
        GpuUpload functions
    */

    // Allocates a buffer, creates a staging buffer for its data and creates a VkWriteDescriptorSet for its descriptor
    // Return the buffer's size or 0 if it fails
    template <typename DataType, typename T = void>
    VkDeviceSize SetupPushDescriptorBuffer(VkDevice device, VmaAllocator allocator,
        PushDescriptorBuffer<T>& pushBuffer, AllocatedBuffer& stagingBuffer,
        size_t elementCount, VkBufferUsageFlags usage, DataType* pData)
    {
        VkDeviceSize bufferSize = elementCount * sizeof(DataType);
        if (bufferSize == 0)
        {
            return 0;
        }
        CreateStorageBufferWithStagingBuffer(allocator, device, pData, pushBuffer.buffer,
            stagingBuffer, usage, bufferSize);
        if (pushBuffer.buffer.bufferHandle == VK_NULL_HANDLE)
        {
            return 0;
        }
        // The push descriptor buffer struct holds its own VkWriteDescriptorSet struct
        WriteBufferDescriptorSets(pushBuffer.descriptorWrite, pushBuffer.bufferInfo,
            pushBuffer.descriptorType, pushBuffer.descriptorBinding,
            pushBuffer.buffer.bufferHandle);
        return bufferSize;
    }

    // This overload of the above function calls the base CreateBuffer function only
    // It does the same thing as the above with the VkWriteDescriptorSet
    template <typename DataType, typename T = void>
    VkDeviceSize SetupPushDescriptorBuffer(VmaAllocator allocator, VmaMemoryUsage memUsage,
        PushDescriptorBuffer<T>& pushBuffer, size_t elementCount,
        VkBufferUsageFlags usage, void* pNextChain = nullptr
    )
    {
        VkDeviceSize bufferSize = sizeof(DataType) * elementCount;
        if (!CreateBuffer(allocator, pushBuffer.buffer, usage,
            memUsage, bufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            return 0;
        }
        // The push descriptor buffer struct holds its own VkWriteDescriptorSet struct
        WriteBufferDescriptorSets(pushBuffer.descriptorWrite, pushBuffer.bufferInfo,
            pushBuffer.descriptorType, pushBuffer.descriptorBinding,
            pushBuffer.buffer.bufferHandle, pNextChain);
        return bufferSize;
    }

    // Calls CreateImage, but also create a VkWriteDescriptorSets struct for the PushDescriptorImage
    uint8_t CreatePushDescriptorImage(VkDevice device, VmaAllocator allocator, PushDescriptorImage& image,
        VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, uint8_t mipLevels,
        VmaMemoryUsage memoryUsage);


    // Creates a global index buffer. Return its size or 0 if it fails
    VkDeviceSize CreateGlobalIndexBuffer(VkDevice device, VmaAllocator allocator, uint8_t bRaytracingSupported,
        AllocatedBuffer& indexStagingBuffer, AllocatedBuffer& indexBuffer, uint32_t* pIndexData, size_t indicesCount);

    uint8_t AllocateTextureDescriptorSet(VkDevice device, uint32_t textureCount, TextureData* pTextures, 
        VkDescriptorPool& descriptorPool, VkDescriptorSetLayout* pLayout, VkDescriptorSet& descriptorSet);

}