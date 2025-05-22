#pragma once
#include "vulkanData.h"
#include "Renderer/Resources/Textures/blitTextures.h"
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
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight, void* pPlatformHandle);

        // Sets up the Vulkan renderer for drawing according to the resources loaded by the engine
        uint8_t SetupForRendering(BlitzenEngine::DrawContext& drawContext);

        // Needed for dx12, not used here for now
        void FinalSetup();

        // Function for DDS texture loading
        uint8_t UploadTexture(void* pData, const char* filepath);

        // Shows a loading screen while waiting for resources to be loaded
        void DrawWhileWaiting(float deltaTime);

        void Update(const BlitzenEngine::DrawContext& context);

        // Called each frame to draw the scene that is requested by the engine
        void DrawFrame(BlitzenEngine::DrawContext& context);

        // When a dynamic object moves, it should call this function to update the staging buffer
        void UpdateObjectTransform(BlitzenEngine::RendererTransformUpdateContext& updateContext);

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
            size_t dynamicTransformDataSize{ 0 };
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

            // Used for transfering data from pre cluster culling pass, to cluster culling pass
            // The buffer changes for transparent objects, 
            // so device addresses to the correct buffer need to be given to the shaders
            AllocatedBuffer clusterDispatchBuffer;
			VkDeviceAddress clusterDispatchBufferAddress;
            AllocatedBuffer clusterCountBuffer;
			VkDeviceAddress clusterCountBufferAddress;
            AllocatedBuffer clusterCountCopyBuffer;
			AllocatedBuffer transparentClusterDispatchBuffer;
			VkDeviceAddress transparentClusterDispatchBufferAddress;
			AllocatedBuffer transparentClusterCountBuffer;
            VkDeviceAddress transparentClusterCountBufferAddress;
			AllocatedBuffer transparentClusterCountCopyBuffer;

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

        BlitzenVulkan::MemoryCrucialHandles m_memoryCrucials;

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
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkRenderingAttachmentInfo m_colorAttachmentInfo{};

        // Depth attachment. Its sampler will be passed to the depth pyramid generation shader, binding 1
        PushDescriptorImage m_depthAttachment
        {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        VkRenderingAttachmentInfo m_depthAttachmentInfo{};

        // The depth pyramid will copy data from the depth attachment to create a pyramid for occlusion culling
        PushDescriptorImage m_depthPyramid
        {
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, VK_IMAGE_LAYOUT_GENERAL
        };
        VkImageView m_depthPyramidMips[ce_maxDepthPyramidMipLevels]{};
        uint8_t m_depthPyramidMipLevels;
        VkExtent2D m_depthPyramidExtent;

        // Will hold all textures that will be loaded for the scene, to pass them to the global descriptor set later
        TextureData loadedTextures[BlitzenCore::Ce_MaxTextureCount];
        size_t textureCount;
        ImageSampler m_textureSampler;

        /*
            Buffer resources section
        */
    private:

        StaticBuffers m_staticBuffers;

        VarBuffers m_varBuffers[ce_framesInFlight];

        /*
            Descriptor section
        */
    private:

        // Layout for descriptors that will be using PushDescriptor extension. Has 10+ bindings
        DescriptorSetLayout m_pushDescriptorBufferLayout;

        VkWriteDescriptorSet m_graphicsDescriptors[Ce_GraphicsDescriptorWriteArraySize]{};
        VkWriteDescriptorSet m_drawCullDescriptors[Ce_ComputeDescriptorWriteArraySize]{};
        //VkWriteDescriptorSet m_clusterCullDescriptors[Ce_Num]{};

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

        // Culling shader for clusters (no mesh shaders)
        PipelineObject m_preClusterCullPipeline;
        PipelineObject m_intialClusterCullPipeline;
        PipelineObject m_transparentClusterCullPipeline;
        PipelineLayout m_clusterCullLayout;

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
        uint32_t windowWidth, uint32_t windowHeight, Queue graphicsQueue, Queue presentQueue, Queue computeQueue,
        VkAllocationCallbacks* pCustomAllocator, Swapchain& newSwapchain, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);

    // Initalizes structure needed to call the DrawWhileWaiting function
    uint8_t CreateIdleDrawHandles(VkDevice device, VkPipeline& pipeline,
        VkPipelineLayout& layout, VkDescriptorSetLayout& setLayout,
        uint32_t queueIndex, VkCommandPool& commandPool, VkCommandBuffer& commandBuffer);



    uint8_t BuildBlas(VkInstance instance, VkDevice device, VmaAllocator vma, VulkanRenderer::FrameTools& frameTools, VkQueue queue,
        BlitzenEngine::DrawContext& context, VulkanRenderer::StaticBuffers& staticBuffers);

    uint8_t BuildTlas(VkInstance instance, VkDevice device, VmaAllocator vma, VulkanRenderer::FrameTools& frameTools, VkQueue queue,
        VulkanRenderer::StaticBuffers& staticBuffers, BlitzenEngine::DrawContext& context);
}