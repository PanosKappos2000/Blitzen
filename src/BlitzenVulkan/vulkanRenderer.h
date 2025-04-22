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

    public:

        // This struct holds any vulkan structure (buffers, sync structures etc), that need to have an instance for each frame in flight
        struct FrameTools
        {
            CommandPool mainCommandPool;
            VkCommandBuffer commandBuffer;

            CommandPool transferCommandPool;
            VkCommandBuffer transferCommandBuffer;

            CommandPool computeCommandPool;
            VkCommandBuffer computeCommandBuffer;

            SyncFence preCulsterCullingFence;
            SyncFence inFlightFence;

            Semaphore imageAcquiredSemaphore;
            Semaphore buffersReadySemaphore;
            Semaphore readyToPresentSemaphore;

            Semaphore preClusterCullingDoneSemaphore;

            uint8_t Init(VkDevice device, Queue graphicsQueue, Queue transferQueue, Queue computeQueue);
        };

        // The renderer will have one instance of these buffers, which will include buffers that my be updated
        struct VarBuffers
        {
            PushDescriptorBuffer<BlitzenEngine::CameraViewData> viewDataBuffer{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };

            PushDescriptorBuffer<void> transformBuffer{ 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            AllocatedBuffer transformStagingBuffer;
            BlitzenEngine::MeshTransform* pTransformData = nullptr;
        };

        struct StaticBuffers
        {
            PushDescriptorBuffer<void> vertexBuffer{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            AllocatedBuffer indexBuffer;

            PushDescriptorBuffer<void> clusterBuffer{ 12, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            PushDescriptorBuffer<void> meshletDataBuffer{ 13, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

            PushDescriptorBuffer<void> materialBuffer{ 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

            AllocatedBuffer renderObjectBuffer;
            VkDeviceAddress renderObjectBufferAddress;
            AllocatedBuffer transparentRenderObjectBuffer;
            VkDeviceAddress transparentRenderObjectBufferAddress;
            PushDescriptorBuffer<void> onpcReflectiveRenderObjectBuffer{ 14, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            VkDeviceAddress onpcRenderObjectBufferAddress;

            PushDescriptorBuffer<void> surfaceBuffer{ 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
            PushDescriptorBuffer<void> lodBuffer{ 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };

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

    public:

        // Vulkan API and memory crucials
        VkInstance m_instance;
        VmaAllocator m_allocator;
        VkDevice m_device;

    private:

        // TODO: Remove this but remember that it might be harder because it calls the below funtions
        // The best would be to pass the Vulkan renderer inside and make the below functions public temporarily
        uint8_t StaticBuffersInit(BlitzenEngine::RenderingResources* pResources);


        // TODO: I should get this out of the class, but we can keep them for now
        uint8_t BuildBlas(const BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces, const BlitCL::DynamicArray<uint32_t>& primitiveVertexCounts);
        uint8_t BuildTlas(BlitzenEngine::RenderObject* pDraws, uint32_t drawCount, BlitzenEngine::MeshTransform* pTransforms, BlitzenEngine::PrimitiveSurface* pSurface);

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
        PipelineObject m_preClusterCullPipeline;

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

    // Initalizes structure needed to call the DrawWhileWaiting function
    uint8_t CreateIdleDrawHandles(VkDevice device, VkPipeline& pipeline,
        VkPipelineLayout& layout, VkDescriptorSetLayout& setLayout,
        uint32_t queueIndex, VkCommandPool& commandPool, VkCommandBuffer& commandBuffer);

    // Creates loading triangle pipeline
    uint8_t CreateLoadingTrianglePipeline(VkDevice device, VkPipeline& pipeline, VkPipelineLayout& layout);



    // Puts command buffer in the ready state. vkCmd type function can be called after this and until vkEndCommandBuffer is called
    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags);

    // Creates semaphore submit info which can be passed to VkSubmitInfo2 before queue submit
    void CreateSemahoreSubmitInfo(VkSemaphoreSubmitInfo& semaphoreInfo, VkSemaphore semaphore, VkPipelineStageFlags2 stage);

    // Ends command buffer and submits it. Synchronization structures can also be specified
    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, uint32_t waitSemaphoreCount = 0,
        VkSemaphoreSubmitInfo* pWaitInfo = nullptr, uint32_t signalSemaphoreCount = 0,
        VkSemaphoreSubmitInfo* signalSemaphore = nullptr, VkFence fence = VK_NULL_HANDLE);

    // Creates rendering attachment info needed for dynamic render pass to begin
    void CreateRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, VkImageView imageView,
        VkImageLayout imageLayout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
        VkClearColorValue clearValueColor = { 0, 0, 0, 0 }, VkClearDepthStencilValue clearValueDepth = { 0, 0 });


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
        if(!CreateStorageBufferWithStagingBuffer(allocator, device, pData, pushBuffer.buffer,
            stagingBuffer, usage, bufferSize))
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