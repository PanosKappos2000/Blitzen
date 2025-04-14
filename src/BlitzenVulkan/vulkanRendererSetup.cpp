#define VMA_IMPLEMENTATION// Implements vma funcions. Header file included in vulkanData.h
#include "vulkanRenderer.h"
#include "vulkanResourceFunctions.h"
#include "vulkanPipelines.h"
#include "Platform/filesystem.h"

namespace BlitzenVulkan
{
	constexpr size_t ce_textureStagingBufferSize = 128 * 1024 * 1024;

    uint8_t VulkanRenderer::UploadTexture(void* pData, const char* filepath) 
    {
        // Checks if resource management has been setup
        if(!m_stats.bResourceManagementReady)
        {
            // Calls the function and checks if it succeeded. If it did not, it fails
            SetupResourceManagement();
            if(!m_stats.bResourceManagementReady)
            {
                BLIT_ERROR("Failed to setup resource management for Vulkan")
                return 0;
            }
        }

        // Creates a big buffer to hold the texture data temporarily. It will pass it later
        // This buffer has a random big size, as it needs to be allocated so that pData is not null in the next function
        BlitzenVulkan::AllocatedBuffer stagingBuffer;
        if(!CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VMA_MEMORY_USAGE_CPU_TO_GPU, ce_textureStagingBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("Failed to create staging buffer for texture data copy")
            return 0;
        }
        pData = stagingBuffer.allocationInfo.pMappedData;

        // Initializes necessary data for DDS texture
		BlitzenEngine::DDS_HEADER header{};
		BlitzenEngine::DDS_HEADER_DXT10 header10{};
        BlitzenPlatform::FileHandle handle{};
        VkFormat format = VK_FORMAT_UNDEFINED;
        if(!BlitzenEngine::OpenDDSImageFile(filepath, header, header10, handle))
        {
            BLIT_ERROR("Failed to open texture file")
            return 0;
        }
		if (!LoadDDSImageData(header, header10, handle, format, pData))
		{
			BLIT_ERROR("Failed to load texture data")
				return 0;
		}

        // Creates the texture image for Vulkan by copying the data from the staging buffer
        if(!CreateTextureImage(stagingBuffer, m_device, m_allocator, loadedTextures[textureCount].image, 
        {header.dwWidth, header.dwHeight, 1}, format, VK_IMAGE_USAGE_SAMPLED_BIT, 
        m_frameToolsList[0].transferCommandBuffer, m_transferQueue.handle, header.dwMipMapCount))
        {
            BLIT_ERROR("Failed to load Vulkan texture image")
            return 0;
        }
        
        // Add the global sampler at the element in the array that was just porcessed
        loadedTextures[textureCount].sampler = m_textureSampler.handle;
        textureCount++;
        return 1;
    }

    bool VulkanRenderer::LoadDDSImageData(BlitzenEngine::DDS_HEADER& header, 
        BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::FileHandle& handle, 
        VkFormat& vulkanImageFormat, void* pData)
    {
        vulkanImageFormat = BlitzenVulkan::GetDDSVulkanFormat(header, header10);

		auto file = reinterpret_cast<FILE*>(handle.pHandle);

        if (vulkanImageFormat == VK_FORMAT_UNDEFINED)
            return false;

        unsigned int blockSize =
            (vulkanImageFormat == VK_FORMAT_BC1_RGBA_UNORM_BLOCK 
                || vulkanImageFormat == VK_FORMAT_BC4_SNORM_BLOCK
                || vulkanImageFormat == VK_FORMAT_BC4_UNORM_BLOCK) ? 
            8 : 16;

        size_t imageSize = BlitzenEngine::GetDDSImageSizeBC(header.dwWidth, header.dwHeight, 
            header.dwMipMapCount, blockSize);

        size_t readSize = fread(pData, 1, imageSize, file);

        if (!pData)
            return false;

        if (readSize != imageSize)
            return false;

        return true;
    }

    void VulkanRenderer::SetupResourceManagement()
    {
        // Initializes VMA allocator which will be used to allocate all Vulkan resources
        // Initialized here since textures can and will be given to Vulkan before the renderer is set up, and they need to be allocated
        if(!CreateVmaAllocator(m_device, m_instance, m_physicalDevice, m_allocator, 
        VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT))
        {
            BLIT_ERROR("Failed to create the vma allocator")
            return;
        }

        auto pMemCrucials = MemoryCrucialHandles::GetInstance();
        pMemCrucials->allocator = m_allocator;
        pMemCrucials->device = m_device;
        pMemCrucials->instance = m_instance;
    
        // This sampler will be used by all textures for now
        // Initialized here since textures can and will be given to Vulkan before the renderer is set up
        m_textureSampler.handle = CreateSampler(m_device, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, 
            VK_SAMPLER_ADDRESS_MODE_REPEAT);
        if(m_textureSampler.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create texture sampler")
            return;
        }

        // Creates command buffers and synchronization structures in the frame tools struct
        // Created on first Vulkan Initialization stage, because can and will be uploaded to Vulkan early
        if (!FrameToolsInit())
        {
            BLIT_ERROR("Failed to create frame tools")
            return;
        }

        // If it goes through everything, it updates the boolean
        m_stats.bResourceManagementReady = 1;
    }

    uint8_t VulkanRenderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources, float& pyramidWidth, float& pyramidHeight)
    {
        // Checks if resource management has been setup
        if(!m_stats.bResourceManagementReady)
        {
            // Retry
            SetupResourceManagement();
            if(!m_stats.bResourceManagementReady)
            {
                BLIT_ERROR("Failed to setup resource management for Vulkan")
                return 0;
            }
        }
        /*
            Color attachment handle and value configuration
        */
        // Color attachment sammpler will be used to copy to the swapchain image
        if(!m_colorAttachment.SamplerInit(m_device, VK_FILTER_NEAREST, 
            VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, nullptr
        ))
        {
            BLIT_ERROR("Failed to create color attachment sampler")
            return 0;
        }
        // Create depth attachment image and image view
        if(!CreatePushDescriptorImage(m_device, m_allocator, m_colorAttachment, 
            {m_drawExtent.width, m_drawExtent.height, 1}, // extent
            ce_colorAttachmentFormat, ce_colorAttachmentImageUsage, 
            1, VMA_MEMORY_USAGE_GPU_ONLY 
        ))
        {
            BLIT_ERROR("Failed to create color attachment image resource")
            return 0;
        }
        // Rendering attachment info, most values stay constant
        CreateRenderingAttachmentInfo(m_colorAttachmentInfo, m_colorAttachment.image.imageView,
            ce_ColorAttachmentLayout, VK_ATTACHMENT_LOAD_OP_LOAD ,VK_ATTACHMENT_STORE_OP_STORE,
            ce_WindowClearColor);
        /*
            Depth attachement value and handle configuration
        */
        // Creates a sampler for the depth attachment, used for the depth pyramid
        VkSamplerReductionModeCreateInfo reductionInfo{};
        reductionInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reductionInfo.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
        if(!m_depthAttachment.SamplerInit(m_device, 
            VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, 
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, &reductionInfo
        ))
        {
            BLIT_ERROR("Failed to create depth attachment sampler")
            return 0;
        }
        // Creates rendering attachment image resource for depth attachment
        if(!CreatePushDescriptorImage(m_device, m_allocator, m_depthAttachment, 
            {m_drawExtent.width, m_drawExtent.height, 1}, 
            ce_depthAttachmentFormat, ce_depthAttachmentImageUsage, 
            1, VMA_MEMORY_USAGE_GPU_ONLY
        ))
        {
            BLIT_ERROR("Failed to create depth attachment image resource")
            return 0;
        }
        // Rendering attachment info info, most values stay constant
        CreateRenderingAttachmentInfo(m_depthAttachmentInfo, m_depthAttachment.image.imageView,
            ce_DepthAttachmentLayout, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, 
            { 0, 0, 0, 0 }, { 0, 0 });
        // Create the depth pyramid image and its mips that will be used for occlusion culling
        if(!CreateDepthPyramid(m_depthPyramid, m_depthPyramidExtent, 
            m_depthPyramidMips, m_depthPyramidMipLevels, // depth pyramid mip view and mip count
            m_drawExtent, m_device, m_allocator
        ))
        {
            BLIT_ERROR("Failed to create the depth pyramid")
            return 0;
        }



        // Creates all know descriptor layouts for all known pipelines
        if(!CreateDescriptorLayouts())
        {
            BLIT_ERROR("Failed to create descriptor set layouts")
            return 0;
        }



        // Creates the uniform buffers
        if(!VarBuffersInit(pResources->transforms))
        {
            BLIT_ERROR("Failed to create uniform buffers")
            return 0;
        }

        // Upload static data to gpu (though some of these might not be static in the future)
        if(!UploadDataToGPU(pResources))
        {
            BLIT_ERROR("Failed to upload data to the GPU")
            return 0;
        }


        
        // Initial draw cull shader compute pipeline
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/InitialDrawCull.comp.glsl.spv", // filepath
            VK_SHADER_STAGE_COMPUTE_BIT, "main", // entry point
            m_drawCullLayout.handle, &m_initialDrawCullPipeline.handle 
        ))
        {
            BLIT_ERROR("Failed to create InitialDrawCull.comp shader program")
            return 0;
        }
        
        // Generate depth pyramid compute shader
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/DepthPyramidGeneration.comp.glsl.spv", 
            VK_SHADER_STAGE_COMPUTE_BIT, "main", // entry point
            m_depthPyramidGenerationLayout.handle, &m_depthPyramidGenerationPipeline.handle
        ))
        {
            BLIT_ERROR("Failed to create DepthPyramidGeneration.comp shader program")
            return 0;
        }
        
        // Late culling shader compute pipeline
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/LateDrawCull.comp.glsl.spv", 
            VK_SHADER_STAGE_COMPUTE_BIT, "main", 
            m_drawCullLayout.handle, &m_lateDrawCullPipeline.handle
        ))
        {
            BLIT_ERROR("Failed to create LateDrawCull.comp shader program")
            return 0;
        }

        // Redundant shader
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/OnpcDrawCull.comp.glsl.spv", // filepath
            VK_SHADER_STAGE_COMPUTE_BIT, "main", m_drawCullLayout.handle, 
            &m_onpcDrawCullPipeline.handle))
        {
            BLIT_ERROR("Failed to create OnpcDrawCull.comp shader program")
            return 0;
        }

        if (!CreateComputeShaderProgram(m_device, "VulkanShaders/TransparentDrawCull.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", m_drawCullLayout.handle,
            &m_transparentDrawCullPipeline.handle))
        {
            BLIT_ERROR("Failed to create OnpcDrawCull.comp shader program");
            return 0;
        }

        // Creates the generate presentation image compute shader program
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/GeneratePresentation.comp.glsl.spv", // filepath
            VK_SHADER_STAGE_COMPUTE_BIT, "main", 
            m_generatePresentationLayout.handle, &m_generatePresentationPipeline.handle   
        ))
        {
            BLIT_ERROR("Failed to create GeneratePresentation.comp shader program")
            return 0;
        }
        
        // Create the graphics pipeline object 
        if(!SetupMainGraphicsPipeline())
        {
            BLIT_ERROR("Failed to create the primary graphics pipeline object")
            return 0;
        }

        // Updates the reference to the depth pyramid width held by the camera
        pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
        pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);

        // Since most of these descriptor writes are static they can be initialized here
        pushDescriptorWritesGraphics[ce_viewDataWriteElement]
            = m_varBuffers[0].viewDataBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[1] = m_currentStaticBuffers.vertexBuffer.descriptorWrite; 
        pushDescriptorWritesGraphics[2] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[3] = m_varBuffers[0].transformBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[4] = m_currentStaticBuffers.materialBuffer.descriptorWrite; 
        pushDescriptorWritesGraphics[5] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[7] = m_currentStaticBuffers.tlasBuffer.descriptorWrite;

        pushDescriptorWritesCompute[ce_viewDataWriteElement] = 
            m_varBuffers[0].viewDataBuffer.descriptorWrite;
        pushDescriptorWritesCompute[1] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[2] = m_varBuffers[0].transformBuffer.descriptorWrite;
        pushDescriptorWritesCompute[3] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
        pushDescriptorWritesCompute[4] = m_currentStaticBuffers.indirectCountBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[5] = m_currentStaticBuffers.visibilityBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[7] = {};// Will hold the depth pyramid for late culling compute shaders

        return 1;
    }

    uint8_t VulkanRenderer::FrameToolsInit()
    {
        VkCommandPoolCreateInfo commandPoolsInfo {};
        commandPoolsInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolsInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;// Allows reset
        commandPoolsInfo.queueFamilyIndex = m_graphicsQueue.index;

		VkCommandPoolCreateInfo dedicatedCommandPoolsInfo{};
		dedicatedCommandPoolsInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		dedicatedCommandPoolsInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        dedicatedCommandPoolsInfo.queueFamilyIndex = m_transferQueue.index;

        VkCommandBufferAllocateInfo commandBuffersInfo{};
        commandBuffersInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBuffersInfo.pNext = nullptr;
        commandBuffersInfo.commandBufferCount = 1;
        commandBuffersInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBufferAllocateInfo dedicatedCmbInfo{};
		dedicatedCmbInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		dedicatedCmbInfo.pNext = nullptr;
		dedicatedCmbInfo.commandBufferCount = 1;
		dedicatedCmbInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceInfo.pNext = nullptr;

        // Semaphore will be the same for both semaphores
        VkSemaphoreCreateInfo semaphoresInfo{};
        semaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoresInfo.flags = 0;
        semaphoresInfo.pNext = nullptr;

        // Creates a set of frame tools for each possible frame in flight
        for(size_t i = 0; i < ce_framesInFlight; ++i)
        {
            FrameTools& frameTools = m_frameToolsList[i];

            // Main command buffer
            VkResult commandPoolResult = vkCreateCommandPool(
                m_device, &commandPoolsInfo, 
                m_pCustomAllocator, &frameTools.mainCommandPool.handle
            );
            if(commandPoolResult != VK_SUCCESS)
                return 0;
            commandBuffersInfo.commandPool = frameTools.mainCommandPool.handle;
            VkResult commandBufferResult = vkAllocateCommandBuffers(
                m_device, &commandBuffersInfo, &frameTools.commandBuffer
            );
            if(commandBufferResult != VK_SUCCESS)
                return 0;

            // Dedicated command buffer
            if (vkCreateCommandPool(m_device, &dedicatedCommandPoolsInfo,
				nullptr, &frameTools.transferCommandPool.handle) != VK_SUCCESS
            )
                return 0;
			dedicatedCmbInfo.commandPool = frameTools.transferCommandPool.handle;
			if (vkAllocateCommandBuffers(m_device, &dedicatedCmbInfo, 
                &frameTools.transferCommandBuffer) != VK_SUCCESS
            )
				return 0;

            // Fence that stops commands from being recorded with a used command buffer
            VkResult fenceResult = vkCreateFence(
                m_device, &fenceInfo, 
                m_pCustomAllocator, &frameTools.inFlightFence.handle
            );
            if(fenceResult != VK_SUCCESS)
                return 0;

			// Samphore that command buffer from being submitted before the image is acquired
            VkResult imageSemaphoreResult = vkCreateSemaphore(
                m_device, &semaphoresInfo, 
                m_pCustomAllocator, &frameTools.imageAcquiredSemaphore.handle
            );
            if(imageSemaphoreResult != VK_SUCCESS)
                return 0;

            // Semaphore that blocks presentation before the commands are all executed
            VkResult presentSemaphoreResult = vkCreateSemaphore(
                m_device, &semaphoresInfo, 
                m_pCustomAllocator, &frameTools.readyToPresentSemaphore.handle
            );
            if(presentSemaphoreResult != VK_SUCCESS)
                return 0;

            // Semaphore to wait for buffer to be updated
			VkResult bufferSemaphoreResult = vkCreateSemaphore(
				m_device, &semaphoresInfo,
				m_pCustomAllocator, &frameTools.buffersReadySemaphore.handle
			);
			if (bufferSemaphoreResult != VK_SUCCESS)
				return 0;
        }

        return 1;
    }

    uint8_t CreateDepthPyramid(PushDescriptorImage& depthPyramidImage, VkExtent2D& depthPyramidExtent, 
        VkImageView* depthPyramidMips, uint8_t& depthPyramidMipLevels, 
        VkExtent2D drawExtent, VkDevice device, VmaAllocator allocator
    )
    {
		const VkFormat depthPyramidFormat = VK_FORMAT_R32_SFLOAT;
        const VkImageUsageFlags depthPyramidUsage =
            VK_IMAGE_USAGE_SAMPLED_BIT | // When a depth pyramid mip is passed as the dst image, it is sampled
            VK_IMAGE_USAGE_STORAGE_BIT; // When a depth pyramid mip is passed as the src image, it is a storage image

    
        // Conservative starting extent
        depthPyramidExtent.width = BlitML::PreviousPow2(drawExtent.width);
        depthPyramidExtent.height = BlitML::PreviousPow2(drawExtent.height);
    
        // Mip level count
		depthPyramidMipLevels = BlitML::GetDepthPyramidMipLevels(depthPyramidExtent.width, depthPyramidExtent.height);
    
        // Creates the primary depth pyramid image
        if(!CreatePushDescriptorImage(device, allocator, depthPyramidImage, 
            {depthPyramidExtent.width, depthPyramidExtent.height, 1}, 
            depthPyramidFormat, depthPyramidUsage, depthPyramidMipLevels, 
            VMA_MEMORY_USAGE_GPU_ONLY
        ))
            return 0;
    
        // Create the mip levels of the depth pyramid
        for(uint8_t i = 0; i < depthPyramidMipLevels; ++i)
        {
            if(!CreateImageView(device, depthPyramidMips[size_t(i)], 
                depthPyramidImage.image.image, depthPyramidFormat, i, 1
            ))
                return 0;
        }
    
        return 1;
    }

    uint8_t VulkanRenderer::CreateDescriptorLayouts()
    {
        /*
            The first descriptor set layout that is going to be configured is the push descriptor set layout.
            It is used by both compute pipelines and graphics pipelines
        */

        // Binding used by both compute and graphics pipelines, to access global data like the view matrix
        VkDescriptorSetLayoutBinding viewDataLayoutBinding{};
        // Binding used by both compute and graphics pipelines, to access the vertex buffer
        VkDescriptorSetLayoutBinding vertexBufferBinding{};
        // Binding used for indirect commands in mesh shaders
        VkDescriptorSetLayoutBinding indirectTaskBufferBinding{};
        // Binding used for surface storage buffer, accessed by either mesh or vertex shader and compute
        VkDescriptorSetLayoutBinding surfaceBufferBinding{};
        // Binding used for clusters. Cluster can be used by either mesh or vertex shader and compute shaders
        VkDescriptorSetLayoutBinding meshletBufferBinding{};
        // Binding used for meshlet indices
        VkDescriptorSetLayoutBinding meshletDataBinding{};

        // Every binding in the pushDescriptorSetLayout will have one descriptor
        constexpr uint32_t descriptorCountOfEachPushDescriptorLayoutBinding = 1;

        // Descriptor set layout binding for view data uniform buffer descriptor
        VkShaderStageFlags viewDataShaderStageFlags = m_stats.meshShaderSupport ? 
            VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT :
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        CreateDescriptorSetLayoutBinding(
            viewDataLayoutBinding, 
            m_varBuffers[0].viewDataBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_varBuffers[0].viewDataBuffer.descriptorType, 
            viewDataShaderStageFlags);

        // Descriptor set layout binding for vertex buffer
        VkShaderStageFlags vertexBufferShaderStageFlags = m_stats.meshShaderSupport ? 
            VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(
            vertexBufferBinding, 
            m_currentStaticBuffers.vertexBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.vertexBuffer.descriptorType, 
            vertexBufferShaderStageFlags);

        // Descriptor set layout binding for surface SSBO
        VkShaderStageFlags surfaceBufferShaderStageFlags = m_stats.meshShaderSupport ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(
            surfaceBufferBinding, 
            m_currentStaticBuffers.surfaceBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.surfaceBuffer.descriptorType, 
            surfaceBufferShaderStageFlags);

        // Descriptor set layout binding for cluster / meshlet SSBO
        VkShaderStageFlags clusterBufferShaderStageFlags = m_stats.meshShaderSupport ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(meshletBufferBinding, 
            m_currentStaticBuffers.meshletBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.meshletBuffer.descriptorType, 
            clusterBufferShaderStageFlags);

        VkShaderStageFlags clusterDataBufferShaderStageFlags = m_stats.meshShaderSupport ? 
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(
            meshletDataBinding, 
            m_currentStaticBuffers.meshletDataBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.meshletDataBuffer.descriptorType, 
            clusterDataBufferShaderStageFlags
        );

        // If mesh shaders are used the bindings needs to be accessed by mesh shaders, otherwise they will be accessed by vertex shader stage
        if(m_stats.meshShaderSupport)
            CreateDescriptorSetLayoutBinding(indirectTaskBufferBinding, 
                m_currentStaticBuffers.indirectTaskBuffer.descriptorBinding, 
                descriptorCountOfEachPushDescriptorLayoutBinding, 
                m_currentStaticBuffers.indirectTaskBuffer.descriptorType, 
                VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT); 

        // Sets the binding for the depth image
        VkDescriptorSetLayoutBinding depthImageBinding{};
        CreateDescriptorSetLayoutBinding(depthImageBinding, 3,
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            VK_SHADER_STAGE_COMPUTE_BIT);

        // Sets the binding for the render object buffer
        VkDescriptorSetLayoutBinding renderObjectBufferBinding{};
        CreateDescriptorSetLayoutBinding(renderObjectBufferBinding, 
            m_currentStaticBuffers.renderObjectBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.renderObjectBuffer.descriptorType, 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        // Set the binding of the transform buffer
        VkDescriptorSetLayoutBinding transformBufferBinding{};
        CreateDescriptorSetLayoutBinding(transformBufferBinding, 
            m_varBuffers[0].transformBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_varBuffers[0].transformBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        // Sets the binding of the material buffer
        VkDescriptorSetLayoutBinding materialBufferBinding{};
        CreateDescriptorSetLayoutBinding(materialBufferBinding, 
            m_currentStaticBuffers.materialBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.materialBuffer.descriptorType, 
            VK_SHADER_STAGE_FRAGMENT_BIT);

        // Sets the binding of the indirect draw buffer
        VkDescriptorSetLayoutBinding indirectDrawBufferBinding{};
        CreateDescriptorSetLayoutBinding(indirectDrawBufferBinding, 
            m_currentStaticBuffers.indirectDrawBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.indirectDrawBuffer.descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);

        // Sets the binding of the indirect count buffer
        VkDescriptorSetLayoutBinding indirectDrawCountBinding{};
        CreateDescriptorSetLayoutBinding(indirectDrawCountBinding, 
            m_currentStaticBuffers.indirectCountBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.indirectCountBuffer.descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT);

        // Sets the binding of the visibility buffer
        VkDescriptorSetLayoutBinding visibilityBufferBinding{};
        CreateDescriptorSetLayoutBinding(visibilityBufferBinding, 
            m_currentStaticBuffers.visibilityBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.visibilityBuffer.descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT);

        // Oblique near plance clipping objects
        VkDescriptorSetLayoutBinding onpcRenderObjectBufferBinding{};
        CreateDescriptorSetLayoutBinding(onpcRenderObjectBufferBinding,
            m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        // Acceleration structure binding 
        VkDescriptorSetLayoutBinding accelerationStructureTlasBinding{};
        CreateDescriptorSetLayoutBinding(accelerationStructureTlasBinding, 
            m_currentStaticBuffers.tlasBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.tlasBuffer.descriptorType, 
            VK_SHADER_STAGE_FRAGMENT_BIT);
        
        // All bindings combined to create the global shader data descriptor set layout
        VkDescriptorSetLayoutBinding shaderDataBindings[15] = 
        {
            viewDataLayoutBinding, 
            vertexBufferBinding, 
            depthImageBinding, 
            renderObjectBufferBinding, 
            transformBufferBinding, 
            materialBufferBinding, 
            indirectDrawBufferBinding, 
            indirectDrawCountBinding, 
            visibilityBufferBinding, 
            surfaceBufferBinding, 
            meshletBufferBinding, 
            meshletDataBinding,
            onpcRenderObjectBufferBinding, 
            accelerationStructureTlasBinding,
            indirectTaskBufferBinding
        };
        m_pushDescriptorBufferLayout.handle = CreateDescriptorSetLayout(m_device, 
            m_stats.bRayTracingSupported ? 
            BLIT_ARRAY_SIZE(shaderDataBindings) - 1 : // Indirect task buffer not added for now
            BLIT_ARRAY_SIZE(shaderDataBindings) - 2, // acceleration structure also removed when raytracing is not active
            shaderDataBindings, 
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if(m_pushDescriptorBufferLayout.handle == VK_NULL_HANDLE)
            return 0;

        /*
            End of descriptor set layout configurations.
        */




        /*
            The next descriptor set layout is the texture descriptor set layout.
            All descriptors / textures held by it will be allocated at setup time.
            Used by the fragment shader(s) only
        */

        // Descriptor set layout for textures
        VkDescriptorSetLayoutBinding texturesLayoutBinding{};
        CreateDescriptorSetLayoutBinding(texturesLayoutBinding, 0,static_cast<uint32_t>(textureCount), 
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_textureDescriptorSetlayout.handle = CreateDescriptorSetLayout(m_device, 1, &texturesLayoutBinding);
        if(m_textureDescriptorSetlayout.handle == VK_NULL_HANDLE)
            return 0;

        /*
            Texture descriptor set layout complete.
        */




        /*
            The descriptor set layout below is for the src and dst images of the debug pyramid.
            Used by the depth generation compute shader only.
        */

        // Binding for input image in depth pyramid creation shader
        VkDescriptorSetLayoutBinding inImageLayoutBinding{};
        CreateDescriptorSetLayoutBinding(inImageLayoutBinding, m_depthPyramid.m_descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, m_depthPyramid.m_descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT);

        // Bindng for output image in depth pyramid creation shader
        VkDescriptorSetLayoutBinding outImageLayoutBinding{};
        CreateDescriptorSetLayoutBinding(
            outImageLayoutBinding, 
            m_depthAttachment.m_descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_depthAttachment.m_descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT
        );

        // Combine the bindings for the final descriptor layout
        VkDescriptorSetLayoutBinding storageImageBindings[2] = 
        {
            inImageLayoutBinding, outImageLayoutBinding
        };
        m_depthPyramidDescriptorLayout.handle = 
            CreateDescriptorSetLayout(m_device, 2, storageImageBindings, 
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if(m_depthPyramidDescriptorLayout.handle == VK_NULL_HANDLE)
            return 0;

        /*
            Depth pyramid descriptor set layout complete.
        */



        /*
            Descriptor used for generate presentation image. Holds the color attachment sampler and the presentation image
        */

        VkDescriptorSetLayoutBinding presentImageLayoutBinding{};
        CreateDescriptorSetLayoutBinding(presentImageLayoutBinding, 0, 1, 
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        VkDescriptorSetLayoutBinding colorAttachmentSamplerLayoutBinding{};
        CreateDescriptorSetLayoutBinding(colorAttachmentSamplerLayoutBinding, 
            m_colorAttachment.m_descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_colorAttachment.m_descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);
        VkDescriptorSetLayoutBinding generatePresentationSetLayoutBindings[2] = 
        {
            presentImageLayoutBinding, 
            colorAttachmentSamplerLayoutBinding
        };
        m_generatePresentationImageSetLayout.handle = CreateDescriptorSetLayout(m_device, 
            2, generatePresentationSetLayoutBindings, 
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR
        );
        if(m_generatePresentationImageSetLayout.handle == VK_NULL_HANDLE)
            return 0;

        /*
            Present compute shader descriptor set layout complete
        */




        /*
            Pipeline layouts. Different combinations of the descriptor set layout and push constants
        */
       
        // Grapchics pipeline layout
        VkDescriptorSetLayout defaultGraphicsPipelinesDescriptorSetLayouts[2] = 
        {
             m_pushDescriptorBufferLayout.handle, // Push descriptor layout
             m_textureDescriptorSetlayout.handle // Texture layout
        };
        VkPushConstantRange globalShaderDataPushContant{};
        CreatePushConstantRange(globalShaderDataPushContant, VK_SHADER_STAGE_VERTEX_BIT,
            sizeof(GlobalShaderDataPushConstant));
        if (!CreatePipelineLayout(m_device, &m_graphicsPipelineLayout.handle,
            2, defaultGraphicsPipelinesDescriptorSetLayouts,
            1, &globalShaderDataPushContant))
        {
            BLIT_ERROR("Failed to create main graphics pipeline layout");
            return 0;
        }

        // Culling shaders shader their laout
        VkPushConstantRange lateCullShaderPostPassPushConstant{};
        CreatePushConstantRange(lateCullShaderPostPassPushConstant,
            VK_SHADER_STAGE_COMPUTE_BIT, sizeof(DrawCullShaderPushConstant));
        if (!CreatePipelineLayout(m_device, &m_drawCullLayout.handle,
            1, &m_pushDescriptorBufferLayout.handle, // push descriptor set layout
            1, &lateCullShaderPostPassPushConstant))
        {
            BLIT_ERROR("Failed to create culling pipeline layout");
            return 0;
        }

        // Layout for depth pyramid generation pipeline
        VkPushConstantRange depthPyramidMipExtentPushConstant{};
        CreatePushConstantRange(
            depthPyramidMipExtentPushConstant, // push constant for image extent
            VK_SHADER_STAGE_COMPUTE_BIT, sizeof(BlitML::vec2) // takes one vec2
        );
        if (!CreatePipelineLayout(
            m_device, &m_depthPyramidGenerationLayout.handle,
            1, &m_depthPyramidDescriptorLayout.handle, // depth pyramid src and dst image push descriptor layout
            1, &depthPyramidMipExtentPushConstant
        ))
            return 0;

        VkPushConstantRange onpcMatrixPushConstant{};
        CreatePushConstantRange(onpcMatrixPushConstant, VK_SHADER_STAGE_VERTEX_BIT, sizeof(BlitML::mat4));
        if(!CreatePipelineLayout(m_device, &m_onpcReflectiveGeometryLayout.handle, 
            2, defaultGraphicsPipelinesDescriptorSetLayouts, // Same descriptors as the default graphics pipeline
            1, &onpcMatrixPushConstant // Push constant for onpc modified projection matrix (temporary)
        ))
            return 0;

        // Generate present image compute shader layout
        VkPushConstantRange colorAttachmentExtentPushConstant{};
        CreatePushConstantRange(colorAttachmentExtentPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, 
            sizeof(BlitML::vec2)
        );
        if(!CreatePipelineLayout(m_device, &m_generatePresentationLayout.handle, 
            1, &m_generatePresentationImageSetLayout.handle, 
            1, &colorAttachmentExtentPushConstant
        ))
            return 0;

        return 1;
    }

    uint8_t VulkanRenderer::VarBuffersInit(BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& transforms)
    {
        for(size_t i = 0; i < ce_framesInFlight; ++i)
        {
            auto& buffers = m_varBuffers[i];

            // Creates the uniform buffer for view data
            if (!CreateBuffer(m_allocator, buffers.viewDataBuffer.buffer,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                sizeof(BlitzenEngine::CameraViewData), VMA_ALLOCATION_CREATE_MAPPED_BIT))
            {
                BLIT_ERROR("Failed to create view data buffer");
                return 0;
            }

            buffers.viewDataBuffer.pData = reinterpret_cast<BlitzenEngine::CameraViewData*>(
                buffers.viewDataBuffer.buffer.allocation->GetMappedData()
            );
            
            WriteBufferDescriptorSets(buffers.viewDataBuffer.descriptorWrite, 
                buffers.viewDataBuffer.bufferInfo, buffers.viewDataBuffer.descriptorType, 
                buffers.viewDataBuffer.descriptorBinding, buffers.viewDataBuffer.buffer.bufferHandle
            );

            auto transformBufferSize
            {
                SetupPushDescriptorBuffer(m_device, m_allocator,buffers.transformBuffer, 
                    buffers.transformStagingBuffer, transforms.GetSize(),
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                    transforms.Data()) 
            };
            if (transformBufferSize == 0)
            {
                BLIT_ERROR("Failed to create transform buffer");
                return 0;
            }

			buffers.pTransformData = reinterpret_cast<BlitzenEngine::MeshTransform*>(
				buffers.transformStagingBuffer.allocationInfo.pMappedData
			);

			BeginCommandBuffer(m_frameToolsList[i].transferCommandBuffer, 
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
            );

			CopyBufferToBuffer(m_frameToolsList[i].transferCommandBuffer,
				buffers.transformStagingBuffer.bufferHandle,
				buffers.transformBuffer.buffer.bufferHandle,
				transformBufferSize, 0, 0
			);

			SubmitCommandBuffer(m_transferQueue.handle, m_frameToolsList[i].transferCommandBuffer);
            vkQueueWaitIdle(m_transferQueue.handle);
        }

        return 1;
    }

    uint8_t VulkanRenderer::UploadDataToGPU(BlitzenEngine::RenderingResources* pResources)
    {
        // The renderer is allow to continue even without render objects but this function should not do anything
        if(pResources->renderObjectCount == 0)
        {
            BLIT_WARN("No objects given to the renderer")
            return 1;
        }

        const auto& vertices = pResources->GetVerticesArray();
        const auto& indices = pResources->GetIndicesArray();
        auto pRenderObjects = pResources->renders;
        auto renderObjectCount = pResources->renderObjectCount;
        const auto& surfaces = pResources->GetSurfaceArray();
        auto pMaterials = pResources->GetMaterialArrayPointer();
        auto materialCount = pResources->GetMaterialCount();
        const auto& meshlets = pResources->GetClusterArray();
        const auto& meshletData = pResources->GetClusterIndices();
        auto pOnpcRenderObjects = pResources->onpcReflectiveRenderObjects;
        auto onpcRenderObjectCount = pResources->onpcReflectiveRenderObjectCount;
		auto& transforms = pResources->transforms;
        const auto& transparentRenderobjects = pResources->GetTranparentRenders();

        // Raytracing support needs additional flags
        uint32_t geometryBuffersRaytracingFlags = m_stats.bRayTracingSupported ?
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            : 0;
        // Creates vertex buffer
        AllocatedBuffer stagingVertexBuffer;
        VkDeviceSize vertexBufferSize
        { 
            SetupPushDescriptorBuffer(m_device, m_allocator,
                m_currentStaticBuffers.vertexBuffer, stagingVertexBuffer, vertices.GetSize(),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT 
                | geometryBuffersRaytracingFlags, vertices.Data()) 
        };
        if (vertexBufferSize == 0)
        {
            BLIT_ERROR("Failed to create vertex buffer");
            return 0;
        }

        // Global index buffer
        AllocatedBuffer stagingIndexBuffer;
        auto indexBufferSize
        { 
            CreateGlobalIndexBuffer(m_device, m_allocator,
            m_stats.bRayTracingSupported, stagingIndexBuffer, m_currentStaticBuffers.indexBuffer,
            indices.Data(), indices.GetSize()) 
        };
        if (indexBufferSize == 0)
        {
            BLIT_ERROR("Failed to create index buffer");
            return 0;
        }

        // Standard render object buffer
        AllocatedBuffer renderObjectStagingBuffer;
        auto renderObjectBufferSize
        {
            SetupPushDescriptorBuffer(m_device, m_allocator,
                m_currentStaticBuffers.renderObjectBuffer, 
                renderObjectStagingBuffer,renderObjectCount, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                pRenderObjects)
        };
        if (renderObjectBufferSize == 0)
        {
            BLIT_ERROR("Failed to create render object buffer");
            return 0;
        }
        m_currentStaticBuffers.renderObjectBufferAddress = GetBufferAddress(m_device,
            m_currentStaticBuffers.renderObjectBuffer.buffer.bufferHandle);

        // Render object buffer that holds objects that use Oblique near plane clipping
        AllocatedBuffer onpcRenderObjectStagingBuffer;
        VkDeviceSize onpcRenderObjectBufferSize = 0;
        if (onpcRenderObjectCount != 0)
        {
            
            onpcRenderObjectBufferSize =
                SetupPushDescriptorBuffer(m_device, m_allocator,
                    m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer,
                    onpcRenderObjectStagingBuffer, onpcRenderObjectCount,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    pOnpcRenderObjects);
            if (onpcRenderObjectBufferSize == 0)
            {
                BLIT_ERROR("Failed to create Oblique Near-Plane Clipping render object buffer");
                return 0;
            }

            m_stats.bObliqueNearPlaneClippingObjectsExist = 1;
        }

        AllocatedBuffer tranparentRenderObjectStagingBuffer;
        VkDeviceSize transparentRenderObjectBufferSize;
        if (transparentRenderobjects.GetSize() != 0)
        {
            transparentRenderObjectBufferSize =
                SetupPushDescriptorBuffer(m_device, m_allocator, 
                    m_currentStaticBuffers.transparentRenderObjectBuffer, 
                    tranparentRenderObjectStagingBuffer, transparentRenderobjects.GetSize(), 
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
                    transparentRenderobjects.Data());
            if (transparentRenderObjectBufferSize == 0)
            {
                BLIT_ERROR("Failed to create transparent render object buffer");
                return 0;
            }

            m_stats.bTranspartentObjectsExist = 1;

            m_currentStaticBuffers.transparentRenderObjectBufferAddress = GetBufferAddress(m_device,
                m_currentStaticBuffers.transparentRenderObjectBuffer.buffer.bufferHandle);
        }

        // Buffer that holds primitives (surfaces that can be drawn)
        AllocatedBuffer surfaceStagingBuffer;
        auto surfaceBufferSize
        {
            SetupPushDescriptorBuffer(m_device, m_allocator,
            m_currentStaticBuffers.surfaceBuffer, 
            surfaceStagingBuffer, surfaces.GetSize(),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            surfaces.Data())
        };
        if (surfaceBufferSize == 0)
        {
            BLIT_ERROR("Failed to create surface buffer");
            return 0;
        }

        // Global material buffer
        AllocatedBuffer materialStagingBuffer; 
        auto materialBufferSize
        {
            SetupPushDescriptorBuffer(m_device, m_allocator,
            m_currentStaticBuffers.materialBuffer,
            materialStagingBuffer, materialCount,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            const_cast<BlitzenEngine::Material*>(pMaterials))
        };
        if (materialBufferSize == 0)
        {
            BLIT_ERROR("Failed to create material buffer");
            return 0;
        }

        // Indirect draw buffer (VkDrawIndexedIndirectCommand holder and render object id)
        auto indirectDrawBufferSize
        {
            SetupPushDescriptorBuffer<IndirectDrawData>(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY,
                m_currentStaticBuffers.indirectDrawBuffer, renderObjectCount,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
        };
        if(indirectDrawBufferSize == 0)
        {
            BLIT_ERROR("Failed to create indirect draw buffer");
            return 0;
        }

        // Indirect count buffer
        if (!SetupPushDescriptorBuffer<uint32_t>(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY,
                m_currentStaticBuffers.indirectCountBuffer, 1, 
                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT 
                | VK_BUFFER_USAGE_TRANSFER_DST_BIT 
                | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
        {
            BLIT_ERROR("Failed to create indirect count buffer");
            return 0;
        }

        // Visiblity buffer for every render object (could be integrated to the render object as 8bit integer)
        auto visibilityBufferSize
        {
            SetupPushDescriptorBuffer<uint32_t>(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY,
                m_currentStaticBuffers.visibilityBuffer, renderObjectCount,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        };
        if(visibilityBufferSize == 0)
        {
            BLIT_ERROR("Failed to create render object visibility buffer");
            return 0;
        }

        
        // Mesh shader indirect commands
        VkDeviceSize indirectTaskBufferSize = sizeof(IndirectTaskData) * renderObjectCount;
        VkDeviceSize meshletBufferSize = sizeof(BlitzenEngine::Meshlet) * meshlets.GetSize();
        AllocatedBuffer meshletStagingBuffer;
        VkDeviceSize meshletDataBufferSize = sizeof(uint32_t) * meshletData.GetSize();
        AllocatedBuffer meshletDataStagingBuffer;

        if(m_stats.meshShaderSupport)
        {
            // Indirect task buffer
            if (indirectTaskBufferSize == 0)
            {
                return 0;
            }
            if (!SetupPushDescriptorBuffer<IndirectTaskData>(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY,
                m_currentStaticBuffers.indirectTaskBuffer, indirectTaskBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
            {
                BLIT_ERROR("Falied to create indirect task buffer");
                return 0;
            }

            // Meshlets / Clusters
            if (meshletBufferSize == 0)
            {
                return 0;
            }
            if (!SetupPushDescriptorBuffer(m_device, m_allocator,
                m_currentStaticBuffers.meshletBuffer, meshletStagingBuffer,
                meshletBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                meshlets.Data()))
            {
                BLIT_ERROR("Failed to create cluster buffer");
                return 0;
            }

            // Cluster indices
            if (meshletDataBufferSize == 0)
            {
                return 0;
            }
            if (!SetupPushDescriptorBuffer(m_device, m_allocator,
                m_currentStaticBuffers.meshletDataBuffer, meshletDataStagingBuffer,
                meshletDataBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                meshletData.Data()))
            {
                BLIT_ERROR("Failed to create cluster indices buffer");
                return 0;
            }
        }

        // Copies the data uploaded to the staging buffer, to the GPU buffers
        CopyStaticBufferDataToGPUBuffers(stagingVertexBuffer.bufferHandle, vertexBufferSize, 
            stagingIndexBuffer.bufferHandle, indexBufferSize, 
            renderObjectStagingBuffer.bufferHandle, renderObjectBufferSize, 
            onpcRenderObjectStagingBuffer.bufferHandle, onpcRenderObjectBufferSize, 
            surfaceStagingBuffer.bufferHandle, surfaceBufferSize, 
            materialStagingBuffer.bufferHandle, materialBufferSize, 
            visibilityBufferSize, meshletStagingBuffer.bufferHandle, meshletBufferSize, 
            meshletDataStagingBuffer.bufferHandle, meshletDataBufferSize);

        // Sets up raytracing acceleration structures, if it is requested and supported
        if(m_stats.bRayTracingSupported)
        {
            if(!BuildBlas(surfaces, pResources->GetPrimitiveVertexCounts()))
                return 0;
            if(!BuildTlas(pRenderObjects, renderObjectCount, transforms.Data(), surfaces.Data()))
                return 0;
        }

        if (!AllocateTextureDescriptorSet(m_device, (uint32_t)textureCount, loadedTextures,
            m_textureDescriptorPool.handle, &m_textureDescriptorSetlayout.handle,
            m_textureDescriptorSet))
        {
            BLIT_ERROR("Failed to allocate texture descriptor sets");
            return 0;
        }

        return 1;
    }

    void VulkanRenderer::CopyStaticBufferDataToGPUBuffers(
        VkBuffer stagingVertexBuffer, VkDeviceSize vertexBufferSize, 
        VkBuffer stagingIndexBuffer, VkDeviceSize indexBufferSize, 
        VkBuffer renderObjectStagingBuffer, VkDeviceSize renderObjectBufferSize, 
        VkBuffer onpcRenderObjectStagingBuffer, VkDeviceSize onpcRenderObjectBufferSize,
        VkBuffer surfaceStagingBuffer, VkDeviceSize surfaceBufferSize, 
        VkBuffer materialStagingBuffer, VkDeviceSize materialBufferSize, 
        VkDeviceSize visibilityBufferSize, 
        VkBuffer clusterStagingBuffer, VkDeviceSize clusterBufferSize, 
        VkBuffer clusterIndicesStagingBuffer, VkDeviceSize clusterIndicesBufferSize)
    {
        // Start recording the transfer commands
        auto& commandBuffer = m_frameToolsList[0].transferCommandBuffer;
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // Vertex data copy
        CopyBufferToBuffer(commandBuffer, stagingVertexBuffer,
            m_currentStaticBuffers.vertexBuffer.buffer.bufferHandle,
            vertexBufferSize, 0, 0);

        // Index data copy
        CopyBufferToBuffer(commandBuffer, stagingIndexBuffer, 
            m_currentStaticBuffers.indexBuffer.bufferHandle, 
            indexBufferSize, 0, 0);

        // Render objects
        CopyBufferToBuffer(commandBuffer, renderObjectStagingBuffer,
            m_currentStaticBuffers.renderObjectBuffer.buffer.bufferHandle,
            renderObjectBufferSize, 0, 0);

        // Render objects that use Oblique Near-Plane Clipping
        if(onpcRenderObjectBufferSize != 0)
            CopyBufferToBuffer(commandBuffer, onpcRenderObjectStagingBuffer,
                m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.buffer.bufferHandle,
                onpcRenderObjectBufferSize, 0, 0);

        // Primitive surfaces
        CopyBufferToBuffer(commandBuffer, surfaceStagingBuffer,
            m_currentStaticBuffers.surfaceBuffer.buffer.bufferHandle, 
            surfaceBufferSize, 0, 0);

        // Materials
        CopyBufferToBuffer(commandBuffer, materialStagingBuffer,
            m_currentStaticBuffers.materialBuffer.buffer.bufferHandle,
            materialBufferSize, 0, 0);

        // Visibility buffer will start with only zeroes. The first frame will do noting, but that should be fine
        vkCmdFillBuffer(commandBuffer, m_currentStaticBuffers.visibilityBuffer.buffer.bufferHandle,
            0, visibilityBufferSize, 0
        );
        
        if(m_stats.meshShaderSupport)
        {
            // Copies the cluster data held by the staging buffer to the meshlet buffer
            CopyBufferToBuffer(commandBuffer, clusterStagingBuffer, 
            m_currentStaticBuffers.meshletBuffer.buffer.bufferHandle, 
            clusterBufferSize, 0, 0);

            CopyBufferToBuffer(commandBuffer, clusterIndicesStagingBuffer, 
            m_currentStaticBuffers.meshletDataBuffer.buffer.bufferHandle, clusterIndicesBufferSize, 
            0, 0);
        }
        
        // Submit the commands and wait for the queue to finish
        SubmitCommandBuffer(m_transferQueue.handle, commandBuffer);
        vkQueueWaitIdle(m_transferQueue.handle);
    }
}