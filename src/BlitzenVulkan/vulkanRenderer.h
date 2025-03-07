#pragma once

#include "vulkanData.h"
#include "Renderer/blitDDSTextures.h"
#include "Game/blitCamera.h"

namespace BlitzenVulkan
{
    class VulkanRenderer
    {
    public:

        VulkanRenderer();

        // Initalizes the Vulkan API. Creates the instance, finds a suitable device for the application's needs, creates surface and swapchain
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight);

        // Sets up the Vulkan renderer for drawing according to the resources loaded by the engine
        uint8_t SetupForRendering(BlitzenEngine::RenderingResources* pResources, float& pyramidWidth, float& pyramidHeight);

        // Function for DDS texture loading
        uint8_t UploadTexture(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10, 
        void* pData, const char* filepath);

        // Called each frame to draw the scene that is requested by the engine
        void DrawFrame(BlitzenEngine::DrawContext& context);

        // This is an incomplete function that attempts to clear anything Vulkan has drawn to swith to a different renderer. It does not work
        void ClearFrame();

        void SetupForSwitch(uint32_t windowWidth, uint32_t windowHeight);

        // I do not do anything on the destructor, but I leave it here because Cleaning Vulkan is peculiar
        ~VulkanRenderer();

        VulkanRenderer operator = (VulkanRenderer) = delete;

    private:

        // This pointer will be used to get access to the renderer for things like automatic destructors
        static VulkanRenderer* m_pThisRenderer;

        // Creates structures that allow Vulkan to allocate resources (command buffers, allocator, texture sampler) 
        void SetupResourceManagement();

        // Defined in vulkanInit. Initializes the frame tools which are handles that need to have one instance for each frame in flight
        uint8_t FrameToolsInit();

        // Initializes the buffers that are included in frame tools
        uint8_t VarBuffersInit();

        // Creates the descriptor set latyouts that are not constant and need to have one instance for each frame in flight
        uint8_t CreateDescriptorLayouts();

        // Takes the data that is to be used in the scene (vertices, primitives, textures etc.) and uploads to the appropriate resource struct
        uint8_t UploadDataToGPU(BlitzenEngine::RenderingResources* pResources);

        // Since the way the graphics pipelines work is fixed and there are only 2 of them, the code is collected in this fixed function
        uint8_t SetupMainGraphicsPipeline();

        uint8_t BuildBlas(BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces, 
        BlitCL::DynamicArray<uint32_t>& primitiveVertexCounts);

        uint8_t BuildTlas(BlitzenEngine::RenderObject* pDraws, uint32_t drawCount, BlitzenEngine::MeshTransform* pTransforms);

        // Dispatches the compute shader that will perform culling and LOD selection and will write to the indirect draw buffer.
        void DispatchRenderObjectCullingComputeShader(VkCommandBuffer commandBuffer, VkPipeline pipeline, 
        uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites, uint32_t drawCount,
        uint8_t lateCulling = 0, uint8_t postPass = 0, uint8_t bOcclusionEnabled = 1, uint8_t bLODs = 1);

        // Handles draw calls using draw indirect commands that should already be set by culling compute shaders
        void DrawGeometry(VkCommandBuffer commandBuffer, 
        VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorWriteCount,
        VkPipeline pipeline, VkPipelineLayout layout, 
        uint32_t drawCount, uint8_t latePass = 0, uint8_t onpcPass = 0, BlitML::mat4* pOnpcMatrix = nullptr);

        // For occlusion culling to be possible a depth pyramid needs to be generated based on the depth attachment
        void GenerateDepthPyramid(VkCommandBuffer commandBuffer);

        // Recreates the swapchain when necessary (and other handles that are involved with the window, like the depth pyramid)
        void RecreateSwapchain(uint32_t windowWidth, uint32_t windowHeight);

    public:

        // Static function that allows access to vulkan renderer at any scope
        inline static VulkanRenderer* GetRendererInstance() {return m_pThisRenderer;}

        inline VulkanStats GetStats() const {return m_stats;}

        /*
            TODO: Switch this to memory crucial handle pointer, to the memory manager
        */
        // The Vulkan API instance
        VkInstance m_instance;
        // Used to allocate vulkan resources like buffers and images
        VmaAllocator m_allocator;
        // Handle to the logical device
        VkDevice m_device;

    /*
        API initialization handles section
    */
    private:

        // Custom allocator, don't need it right now
        VkAllocationCallbacks* m_pCustomAllocator = nullptr;

        VkDebugUtilsMessengerEXT m_debugMessenger;

        // It's important for this to be declared above swapchain, as it needs to be destroyed after it
        SurfaceKHR m_surface;

        VkPhysicalDevice m_physicalDevice;

        Swapchain m_swapchainValues;

        // All the queue handles and indices retrieved from the device on initialization
        Queue m_graphicsQueue;
        Queue m_presentQueue;
        Queue m_computeQueue;

    /*
        Image resources section
    */
    private:

        // Data for the rendering attachments
        AllocatedImage m_colorAttachment;
        AllocatedImage m_depthAttachment;
        VkExtent2D m_drawExtent;

        // The depth pyramid is generated for occlusion culling based on the depth buffer of an initial render pass
        // It has a maximum fo 16 mip levels, but the actual count is based on the window width and height
        AllocatedImage m_depthPyramid;
        VkImageView m_depthPyramidMips[16];
        uint8_t m_depthPyramidMipLevels = 0;

        VkExtent2D m_depthPyramidExtent;
        ImageSampler m_depthAttachmentSampler;

        // Array of structs that represent the way textures will be pushed to the GPU
        TextureData loadedTextures[BlitzenEngine::ce_maxTextureCount];
        size_t textureCount = 0;
        ImageSampler m_textureSampler;

    /*
        Buffer resources section
    */
    private:

        // Holds data for buffers that will be loaded once and will be used for every object
        struct StaticBuffers
        {
            // The vertex buffer is a storage buffer that will be part of the push descriptor layout at binding 1
            // It will hold all vertices for all the objects on the scene
            PushDescriptorBuffer<void> vertexBuffer{1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            AllocatedBuffer indexBuffer;

            // The meshlet / cluster buffer is a storage buffer that will be part of the push descirptor layout at binding 12
            // It will hold all meshlets for all the primitives on the scene
            PushDescriptorBuffer<void> meshletBuffer{12, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The meshlet data buffer is a storage buffer that will be part of the push descirptor layout at binding 13
            // It will hold indices to access the correct cluster in the meshlet buffer
            PushDescriptorBuffer<void> meshletDataBuffer{13, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The material buffer is a storage buffer that will be part of the push descriptor layout at binding 6
            // It will hold all the materials used in the scene
            PushDescriptorBuffer<void> materialBuffer{6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The render object buffer is a storage buffer that will be part of the push descriptor layout at binding 4
            // It will holds indices to the primitive and transform of all the render object in the the scene
            PushDescriptorBuffer<void> renderObjectBuffer{4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The surface buffer is a storage buffer that will be part of the push descriptor layout at binding 2
            // It will hold the data for all the primitive surfaces that will be used in the scene
            PushDescriptorBuffer<void> surfaceBuffer{2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The transform buffer is a storage buffer that will be part of the push descriptor layout at bidning 5
            // It will hold the transforms of all the objects in the scene
            PushDescriptorBuffer<void> transformBuffer{5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The indirect draw buffer is a storage buffer that will be part of the push descriptor layout at binding 7
            // It will hold all the indirect draw commands for each frame
            PushDescriptorBuffer<void> indirectDrawBuffer{7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The indirect task buffer is a storage buffer that will be part of the push descriptor layout at binding 8
            // It will hold all the indirect task commands for each frame
            PushDescriptorBuffer<void> indirectTaskBuffer{8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The indirect draw count buffer is a storage buffer that will be part of the push descriptor layout at binding 9
            // It will hold one integer that will tell the graphics pipeline how many elements it should access in the indirect draw buffer
            PushDescriptorBuffer<void> indirectCountBuffer{9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The visibility buffer is a storage buffer thta will be part of the push descriptor layout at binding 10
            // It will hold either 1 or 0 for each object based on if they were visible last frame or not
            PushDescriptorBuffer<void> visibilityBuffer{10, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};

            // The onpcReflectiveRenderObject buffer is a storage buffer that will be part of the push descriptor layout at binding 14
            // It will hold the objects that use Oblique near-plane clipping
            PushDescriptorBuffer<void> onpcReflectiveRenderObjectBuffer{14, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
            
            // Raytracing primitive acceleration structure buffer and vulkan objects
            AllocatedBuffer blasBuffer;
            BlitCL::DynamicArray<AccelerationStructure> blasData;

            // Raytracing instance acceleration structure buffer and vulkan objects
            AllocatedBuffer tlasBuffer;
            AccelerationStructure tlasData;
        };
        StaticBuffers m_currentStaticBuffers;

        struct VarBuffers
        {
            // The global view data buffer is a uniform buffer that will be part of the push descriptor layout at binding 0
            // It will hold view data like the view matrix or frustum planes data
            PushDescriptorBuffer<BlitzenEngine::CameraViewData> viewDataBuffer{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
        };
        VarBuffers m_varBuffers[ce_framesInFlight];

    /*
        Descriptor section
    */
    private:

        // Layout for descriptors that will be using PushDescriptor extension. Has 10+ bindings
        DescriptorSetLayout m_pushDescriptorBufferLayout;

        VkWriteDescriptorSet pushDescriptorWritesGraphics[7];
        VkWriteDescriptorSet pushDescriptorWritesCompute[8];
        VkWriteDescriptorSet m_pushDescriptorWritesOnpcGraphics[7];
        VkWriteDescriptorSet m_pushDescriptorWritesOnpcCompute[8];

        // Layout for descriptor set that passes the source image and dst image for each depth pyramid mip
        DescriptorSetLayout m_depthPyramidDescriptorLayout;

        // Descriptor layout for the texture descriptors. 1 binding that holds an array of textures
        DescriptorSetLayout m_textureDescriptorSetlayout;

        // This descriptor set does not use push descriptors and thus it needs to be allocated with a descriptor pool
        DescriptorPool m_textureDescriptorPool;
        VkDescriptorSet m_textureDescriptorSet;

        DescriptorSetLayout m_backgroundImageSetLayout;

    /*
        Pipelines section
    */
    private:

        // graphics pipeline object and lyaouts
        PipelineObject m_opaqueGeometryPipeline;
        PipelineObject m_postPassGeometryPipeline;
        PipelineLayout m_graphicsPipelineLayout;

        // Oblique Near-Plane clipping objects
        PipelineObject m_onpcReflectiveGeometryPipeline;
        PipelineLayout m_onpcReflectiveGeometryLayout;
        PipelineObject m_onpcDrawCullPipeline;
        PipelineLayout m_onpcDrawCullLayout;

        // Draws opaque geometries. Uses opaque geometry pipline object and graphics pipline layout
        PipelineProgram m_opaqueGeometryProgram;

        // Draw post pass geometries(geometries with alpha discard). Uses post pass geometry pipline and graphics pipeline layout
        PipelineProgram m_postPassGeometryProgram;

        // These are compute pipelines that hold the shaders that will perform culling operations on render object level
        // @InitialDrawCull
        // The initial culling shader will do frustum culling tests on the objects that were visible last frame
        // @LateDrawCull
        // The late culling shader will do both frustum and occlusion culling on all objects
        // For objects that were not accessed by the initial shader, it will create draw commands (if the are not culled away)
        // It will also set the frame visibility of each object. This data will be accessed by the initial cull shader next frame
        PipelineObject m_initialDrawCullPipeline;
        PipelineObject m_lateDrawCullPipeline;
        PipelineLayout m_drawCullLayout;

        // Performs furstum culling and LOD selection on object that were tagged visible last frame by the late draw cull
        // Sets up an indirect draw command buffer and indirect count buffer for the graphics programs to use
        PipelineProgram m_initialDrawCullProgram;

        // Performs frustum culling and occlusion culling and LOD selection on all objects
        // Sets up an indirect draw command buffer and indirect count buffer for objects that were not tagged as visible last frame
        PipelineProgram m_lateDrawCullProgram;

        // The depth pyramid generation pipeline will hold a helper compute shader for the late culling pipeline.
        // It will generate the depth pyramid from the 1st pass' depth buffer. It will then be used for occlusion culling 
        PipelineObject m_depthPyramidGenerationPipeline;
        PipelineLayout m_depthPyramidGenerationLayout;

        // Used when draw count is 0 to generate a default background (might use it with normal drawing as well in the future)
        PipelineObject m_basicBackgroundPipeline;
        PipelineLayout m_basicBackgroundLayout;
    
    /*
        Runtime section
    */
    private:

        // This struct holds any vulkan structure (buffers, sync structures etc), that need to have an instance for each frame in flight
        struct FrameTools
        {
            CommandPool mainCommandPool;
            VkCommandBuffer commandBuffer;

            SyncFence inFlightFence;
            Semaphore imageAcquiredSemaphore;
            Semaphore readyToPresentSemaphore;        
        };
        FrameTools m_frameToolsList[ce_framesInFlight];

        // Used to access the right frame tools depending on which ones are already being used
        uint8_t m_currentFrame = 0;

        // Holds stats that give information about how the vulkanRenderer is operating
        VulkanStats m_stats;
    };




    // Creates the Vulkan instance, required to interface with the Vulkan API
    uint8_t CreateInstance(VkInstance& instance, VkDebugUtilsMessengerEXT* pDM = nullptr);

    // Checks if the requested validation layers are supported
    uint8_t EnableInstanceValidation(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo);

    uint8_t EnabledInstanceSynchronizationValidation();



    // Picks a suitable physical device (GPU) for the application and passes the handle to the first argument.
    // Returns 1 if if finds a fitting device. Expects the instance and surface arguments to be valid
    uint8_t PickPhysicalDevice(VkPhysicalDevice& gpu, VkInstance instance, VkSurfaceKHR surface,
    Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, 
    VulkanStats& stats);

    // Validates the suitability of a GPU for the application
    uint8_t ValidatePhysicalDevice(VkPhysicalDevice pdv, VkInstance instance, VkSurfaceKHR surface, 
    Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, 
    VulkanStats& stats);

    // Looks for extensions in a physical device, returns 1 if everything goes well, and uploads data to stats and the extension count
    uint8_t LookForRequestedExtensions(VkPhysicalDevice gpu, VulkanStats& stats);

    // Creates the logical device, after activating required extensions and features
    uint8_t CreateDevice(VkDevice& device, VkPhysicalDevice physicalDevice, Queue& graphicsQueue, 
    Queue& presentQueue, Queue& computeQueue, VulkanStats& stats);
    


    // Creates the swapchain
    uint8_t CreateSwapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice,
    uint32_t windowWidth, uint32_t windowHeight, Queue graphicsQueue, Queue presentQueue, Queue computeQueue, 
    VkAllocationCallbacks* pCustomAllocator, Swapchain& newSwapchain, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);

    uint8_t FindSwapchainSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info, 
    VkFormat& swapchainFormat);

    uint8_t FindSwapchainPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info);

    uint8_t FindSwapchainSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info, 
    Swapchain& swapchain);



    /* ----------------------
        Vulkan Resources 
    ------------------------- */

    // Create a allocator from the VMA library
    uint8_t CreateVmaAllocator(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VmaAllocator& allocator, 
    VmaAllocatorCreateFlags flags);

    // Creates the depth pyramid image and mip levels and their data. Needed for occlusion culling
    uint8_t CreateDepthPyramid(AllocatedImage& depthPyramidImage, VkExtent2D& depthPyramidExtent, 
    VkImageView* depthPyramidMips, uint8_t& depthPyramidMipLevels, VkSampler& depthAttachmentSampler, 
    VkExtent2D drawExtent, VkDevice device, VmaAllocator allocator, uint8_t createSampler = 1);

    // Allocates a buffer using VMA
    uint8_t CreateBuffer(VmaAllocator allocator, AllocatedBuffer& buffer, VkBufferUsageFlags bufferUsage, 
    VmaMemoryUsage memoryUsage, VkDeviceSize bufferSize, VmaAllocationCreateFlags allocationFlags);

    // Create a gpu only storage buffer and a staging buffer to hold its data
    // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set, the pAddr pointer is updated as well
    VkDeviceAddress CreateStorageBufferWithStagingBuffer(
    VmaAllocator allocator, VkDevice device, 
    void* pData, AllocatedBuffer& storageBuffer, AllocatedBuffer& stagingBuffer, 
    VkBufferUsageFlags usage, VkDeviceSize size);

    // Returns the GPU address of a buffer
    VkDeviceAddress GetBufferAddress(VkDevice device, VkBuffer buffer);

    // Creates and image with VMA and also create an image view for it
    uint8_t CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, 
    VkFormat format, VkImageUsageFlags usage, uint8_t mipLevels = 1, VmaMemoryUsage memoryUsage =VMA_MEMORY_USAGE_GPU_ONLY);

    // Creates an image view. Called automatically by CreateImage but can also be used seperately for something like the depth pyramid
    uint8_t CreateImageView(VkDevice device, VkImageView& imageView, VkImage image, VkFormat format, uint8_t baseMipLevel, uint8_t mipLevels);

    // Allocate an image resource to be used specifically as texture. 
    // The 1st parameter should be the loaded image data that should be passed to the image resource
    // This function should not be used if possible, the one below is overall better
    void CreateTextureImage(void* data, VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, 
    VkFormat format, VkImageUsageFlags usage, VkCommandBuffer commandBuffer, VkQueue queue, uint8_t mipLevels = 1);

    // This function is similar to the above but it gives its own buffer and mip levels are required. 
    // The buffer should already hold the texture data in pMappedData
    uint8_t CreateTextureImage(AllocatedBuffer& buffer, VkDevice device, VmaAllocator allocator, AllocatedImage& image, 
    VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, VkCommandBuffer commandBuffer, VkQueue queue, uint8_t mipLevels);

    // Placeholder sampler creation function. Used for the default sampler used by all textures so far. 
    // TODO: Replace this with a general purpose function
    uint8_t CreateTextureSampler(VkDevice device, VkSampler& sampler, VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR);

    // Returns VkFormat based on DDS input to correctly load a texture image
    VkFormat GetDDSVulkanFormat(const BlitzenEngine::DDS_HEADER& header, const BlitzenEngine::DDS_HEADER_DXT10& header10);

    // Returns a VkSampler used only for depth pyramid creation
    // TODO: Replace this with a general purpose function like the above
    VkSampler CreateSampler(VkDevice device, VkSamplerReductionMode reductionMode);

    // Uses vkCmdBlitImage2 to copy a source image to a destination image. Hardcodes alot of parameters. 
    //Can be improved but this is used rarely for now, so I will leave it as is until I have to
    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, 
    VkImage dstImage, VkImageLayout dstLayout, VkExtent2D srcImageSize, VkExtent2D dstImageSize, 
    VkImageSubresourceLayers srcImageSL, VkImageSubresourceLayers dstImageSL, VkFilter filter);

    // Copies parts of one buffer to parts of another, depending on the offsets that are passed
    void CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, 
    VkDeviceSize copySize, VkDeviceSize srcOffset, VkDeviceSize dstOffset);

    // Copies data held by a buffer to an image. Used in texture image creation to hold the texture data in the buffer and then pass it to the image
    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage image, VkImageLayout imageLayout, VkExtent3D extent);

    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout imageLayout, 
    uint32_t bufferImageCopyRegionCount, VkBufferImageCopy2* bufferImageCopyRegions);

    // Create a copy VkBufferImageCopy2 to be passed to the array that will be passed to CopyBufferToImage function above
    void CreateCopyBufferToImageRegion(VkBufferImageCopy2& result, VkExtent3D imageExtent, VkOffset3D imageOffset, 
    VkImageAspectFlags aspectMask, uint32_t mipLevel, uint32_t baseArrayLayer, uint32_t layerCount, VkDeviceSize bufferOffset, 
    uint32_t bufferImageHeight, uint32_t bufferRowLength);

    // Creates a descriptor pool for descriptor sets whose memory should be managed by one and are not push descriptors (managed by command buffer)
    VkDescriptorPool CreateDescriptorPool(VkDevice device, uint32_t poolSizeCount, VkDescriptorPoolSize* pPoolSizes, uint32_t maxSets);

    // Allocates one or more descriptor sets whose memory will be managed by a descriptor pool
    uint8_t AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout* pLayouts, 
    uint32_t descriptorSetCount, VkDescriptorSet* pSets);

    // Creates VkWriteDescriptorSet for a buffer type descriptor set
    void WriteBufferDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorBufferInfo& bufferInfo, 
    VkDescriptorType descriptorType, uint32_t dstBinding, VkBuffer buffer, 
    VkDescriptorSet dstSet = VK_NULL_HANDLE,  VkDeviceSize offset = 0, uint32_t descriptorCount = 1,
    VkDeviceSize range = VK_WHOLE_SIZE, uint32_t dstArrayElement = 0);

    // Creates VkWriteDescirptorSet for an image type descirptor set. The image info struct(s) need to be initialized outside
    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo* pImageInfos, VkDescriptorType descriptorType, VkDescriptorSet dstSet, 
    uint32_t descriptorCount, uint32_t binding, uint32_t dstArrayElement = 0);

    // Creates VkDescriptorImageInfo and uses it to create a VkWriteDescriptorSet for images. DescriptorCount is set to 1 by default
    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo& imageInfo, 
    VkDescriptorType descirptorType, VkDescriptorSet dstSet, 
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
    uint8_t CreateShaderProgram(const VkDevice& device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkShaderModule& shaderModule, VkPipelineShaderStageCreateInfo& pipelineShaderStage, VkSpecializationInfo* pSpecializationInfo = nullptr);

    // Tries to create a compute pipeline
    uint8_t CreateComputeShaderProgram(VkDevice device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkPipelineLayout& layout, VkPipeline* pPipeline, VkSpecializationInfo* pSpecializationInfo = nullptr);

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
    uint8_t CreatePipelineLayout(VkDevice device, VkPipelineLayout* layout, uint32_t descriptorSetLayoutCount, 
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




    /*
        Template functions
    */

    // This function calls the CreateStorageBufferWithStagingBuffer function for a PushDescriptorBuffer
    // It also create a VkWriteDescriptorSets struct for it
    template <typename T = void>
    uint8_t SetupPushDescriptorBuffer(VkDevice device, VmaAllocator allocator, 
    PushDescriptorBuffer<T>& pushBuffer, AllocatedBuffer& stagingBuffer, 
    VkDeviceSize bufferSize, VkBufferUsageFlags usage, void* pData)
    {
        // Creates the storage buffer (does not save the address, it assumes the caller does not need it)
        // It also creates its staging buffer. The caller can later use it to pass the data to the storage buffer
        CreateStorageBufferWithStagingBuffer(allocator, device, pData, pushBuffer.buffer, 
        stagingBuffer, usage, bufferSize);
        if(pushBuffer.buffer.bufferHandle == VK_NULL_HANDLE)
            return 0;
        
        // The push descriptor buffer struct holds its own VkWriteDescriptorSet struct
        WriteBufferDescriptorSets(
            pushBuffer.descriptorWrite, 
            pushBuffer.bufferInfo, 
            pushBuffer.descriptorType, 
            pushBuffer.descriptorBinding, 
            pushBuffer.buffer.bufferHandle
        );
        return 1;
    }

    // This overload of the above function calls the base CreateBuffer function only
    // It does the same thing as the above with the VkWriteDescriptorSet
    template <typename T = void>
    uint8_t SetupPushDescriptorBuffer(VmaAllocator allocator, VmaMemoryUsage memUsage,
    PushDescriptorBuffer<T>& pushBuffer, VkDeviceSize bufferSize, VkBufferUsageFlags usage)
    {
        if(!CreateBuffer(allocator, pushBuffer.buffer, usage, memUsage, bufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
            return 0;
        
        // The push descriptor buffer struct holds its own VkWriteDescriptorSet struct
        WriteBufferDescriptorSets(
            pushBuffer.descriptorWrite, 
            pushBuffer.bufferInfo, 
            pushBuffer.descriptorType, 
            pushBuffer.descriptorBinding, 
            pushBuffer.buffer.bufferHandle
        );
        return 1;
    }
}

namespace BlitzenCore
{
    // Gets the memory crucial handles form the memory manager (defined in blitzenMemory.cpp)
    BlitzenVulkan::MemoryCrucialHandles* GetVulkanMemoryCrucials();
}