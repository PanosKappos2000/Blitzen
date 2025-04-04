#define VMA_IMPLEMENTATION// Implements vma funcions. Header file included in vulkanData.h
#include "vulkanRenderer.h"

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
        unsigned int format = VK_FORMAT_UNDEFINED;
        if(!BlitzenEngine::LoadDDSImage(filepath, header, header10, format, 
            BlitzenEngine::RendererToLoadDDS::Vulkan, pData
        ))
        {
            BLIT_ERROR("Failed to load texture image")
            return 0;
        }
        // Casts the placeholder format to a VkFormat
        auto vkFormat{ static_cast<VkFormat>(format) };

        // Creates the texture image for Vulkan by copying the data from the staging buffer
        if(!CreateTextureImage(stagingBuffer, m_device, m_allocator, loadedTextures[textureCount].image, 
        {header.dwWidth, header.dwHeight, 1}, vkFormat, VK_IMAGE_USAGE_SAMPLED_BIT, 
        m_frameToolsList[0].commandBuffer, m_graphicsQueue.handle, header.dwMipMapCount))
        {
            BLIT_ERROR("Failed to load Vulkan texture image")
            return 0;
        }
        
        // Add the global sampler at the element in the array that was just porcessed
        loadedTextures[textureCount].sampler = m_textureSampler.handle;
        textureCount++;
        return 1;
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

        MemoryCrucialHandles* pMemCrucials = BlitzenCore::GetVulkanMemoryCrucials();
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
            VK_SHADER_STAGE_COMPUTE_BIT, "main", // entry point
            m_drawCullLayout.handle, &m_onpcDrawCullPipeline.handle 
        ))
        {
            BLIT_ERROR("Failed to create OnpcDrawCull.comp shader program")
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

        // This is obsolete but I am not in the mood to fix it
        m_pushDescriptorWritesOnpcGraphics[0] = {};
        m_pushDescriptorWritesOnpcGraphics[1] = m_currentStaticBuffers.vertexBuffer.descriptorWrite; 
        m_pushDescriptorWritesOnpcGraphics[2] = m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.descriptorWrite;
        m_pushDescriptorWritesOnpcGraphics[3] = m_varBuffers[0].transformBuffer.descriptorWrite;
        m_pushDescriptorWritesOnpcGraphics[4] = m_currentStaticBuffers.materialBuffer.descriptorWrite; 
        m_pushDescriptorWritesOnpcGraphics[5] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
        m_pushDescriptorWritesOnpcGraphics[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;
		m_pushDescriptorWritesOnpcGraphics[7] = m_currentStaticBuffers.tlasBuffer.descriptorWrite;

        m_pushDescriptorWritesOnpcCompute[0] = {};// This will be where the global shader data write will be, but this one is not always static
        m_pushDescriptorWritesOnpcCompute[1] = m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.descriptorWrite; 
        m_pushDescriptorWritesOnpcCompute[2] = m_varBuffers[0].transformBuffer.descriptorWrite;
        m_pushDescriptorWritesOnpcCompute[3] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
        m_pushDescriptorWritesOnpcCompute[4] = m_currentStaticBuffers.indirectCountBuffer.descriptorWrite; 
        m_pushDescriptorWritesOnpcCompute[5] = m_currentStaticBuffers.visibilityBuffer.descriptorWrite; 
        m_pushDescriptorWritesOnpcCompute[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;
        m_pushDescriptorWritesOnpcCompute[7] = {}; 

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
            viewDataShaderStageFlags
        );

        // Descriptor set layout binding for vertex buffer
        VkShaderStageFlags vertexBufferShaderStageFlags = m_stats.meshShaderSupport ? 
            VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(
            vertexBufferBinding, 
            m_currentStaticBuffers.vertexBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.vertexBuffer.descriptorType, 
            vertexBufferShaderStageFlags
        );

        // Descriptor set layout binding for surface SSBO
        VkShaderStageFlags surfaceBufferShaderStageFlags = m_stats.meshShaderSupport ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(
            surfaceBufferBinding, 
            m_currentStaticBuffers.surfaceBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.surfaceBuffer.descriptorType, 
            surfaceBufferShaderStageFlags
        );

        // Descriptor set layout binding for cluster / meshlet SSBO
        VkShaderStageFlags clusterBufferShaderStageFlags = m_stats.meshShaderSupport ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(
            meshletBufferBinding, 
            m_currentStaticBuffers.meshletBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.meshletBuffer.descriptorType, 
            clusterBufferShaderStageFlags
        );

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
            CreateDescriptorSetLayoutBinding(
                indirectTaskBufferBinding, 
                m_currentStaticBuffers.indirectTaskBuffer.descriptorBinding, 
                descriptorCountOfEachPushDescriptorLayoutBinding, 
                m_currentStaticBuffers.indirectTaskBuffer.descriptorType, 
                VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT
            ); 

        // Sets the binding for the depth image
        VkDescriptorSetLayoutBinding depthImageBinding{};
        CreateDescriptorSetLayoutBinding(
            depthImageBinding, 
            3, // Binding ID, the depth image binding is never specified before this point
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            VK_SHADER_STAGE_COMPUTE_BIT);

        // Sets the binding for the render object buffer
        VkDescriptorSetLayoutBinding renderObjectBufferBinding{};
        CreateDescriptorSetLayoutBinding(
            renderObjectBufferBinding, 
            m_currentStaticBuffers.renderObjectBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.renderObjectBuffer.descriptorType, 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT
        );

        // Set the binding of the transform buffer
        VkDescriptorSetLayoutBinding transformBufferBinding{};
        CreateDescriptorSetLayoutBinding(
            transformBufferBinding, 
            m_varBuffers[0].transformBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_varBuffers[0].transformBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT
        );

        // Sets the binding of the material buffer
        VkDescriptorSetLayoutBinding materialBufferBinding{};
        CreateDescriptorSetLayoutBinding(
            materialBufferBinding, 
            m_currentStaticBuffers.materialBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.materialBuffer.descriptorType, 
            VK_SHADER_STAGE_FRAGMENT_BIT
        );

        // Sets the binding of the indirect draw buffer
        VkDescriptorSetLayoutBinding indirectDrawBufferBinding{};
        CreateDescriptorSetLayoutBinding(
            indirectDrawBufferBinding, 
            m_currentStaticBuffers.indirectDrawBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.indirectDrawBuffer.descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT
        );

        // Sets the binding of the indirect count buffer
        VkDescriptorSetLayoutBinding indirectDrawCountBinding{};
        CreateDescriptorSetLayoutBinding(
            indirectDrawCountBinding, 
            m_currentStaticBuffers.indirectCountBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.indirectCountBuffer.descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT
        );

        // Sets the binding of the visibility buffer
        VkDescriptorSetLayoutBinding visibilityBufferBinding{};
        CreateDescriptorSetLayoutBinding(
            visibilityBufferBinding, 
            m_currentStaticBuffers.visibilityBuffer.descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.visibilityBuffer.descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT
        );

        // Oblique near plance clipping objects
        VkDescriptorSetLayoutBinding onpcRenderObjectBufferBinding{};
        CreateDescriptorSetLayoutBinding(
            onpcRenderObjectBufferBinding,
            m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT
        );

        // Acceleration structure binding 
        VkDescriptorSetLayoutBinding accelerationStructureTlasBinding{};
        CreateDescriptorSetLayoutBinding(
            accelerationStructureTlasBinding, 
            m_currentStaticBuffers.tlasBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.tlasBuffer.descriptorType, 
            VK_SHADER_STAGE_FRAGMENT_BIT
        );
        
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
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR
        );
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
        CreateDescriptorSetLayoutBinding(
            texturesLayoutBinding, 
            0, // binding ID
            static_cast<uint32_t>(textureCount), // bidning count is the same as the texture count
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            VK_SHADER_STAGE_FRAGMENT_BIT
        );
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
        CreateDescriptorSetLayoutBinding(
            inImageLayoutBinding, 
            m_depthPyramid.m_descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_depthPyramid.m_descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT
        );

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
        m_depthPyramidDescriptorLayout.handle = CreateDescriptorSetLayout(
            m_device, 2, storageImageBindings, 
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR
        );
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
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT
        );
        VkDescriptorSetLayoutBinding colorAttachmentSamplerLayoutBinding{};
        CreateDescriptorSetLayoutBinding(colorAttachmentSamplerLayoutBinding, 
            m_colorAttachment.m_descriptorBinding, 
            descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_colorAttachment.m_descriptorType, VK_SHADER_STAGE_COMPUTE_BIT
        );
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
        if(!CreatePipelineLayout(
            m_device, &m_graphicsPipelineLayout.handle, 
            2, defaultGraphicsPipelinesDescriptorSetLayouts, // descriptor set layouts
            0, nullptr // no push constants
        ))
            return 0;

        // Culling shaders shader their laout
        VkPushConstantRange lateCullShaderPostPassPushConstant{};
        CreatePushConstantRange(
            lateCullShaderPostPassPushConstant, // push constant post pass boolean
            VK_SHADER_STAGE_COMPUTE_BIT, 
            sizeof(DrawCullShaderPushConstant)
        );
        if(!CreatePipelineLayout(
            m_device, &m_drawCullLayout.handle, 
            1, &m_pushDescriptorBufferLayout.handle, // push descriptor set layout
            1, &lateCullShaderPostPassPushConstant // push constant only used by late culling pipeline
        ))
            return 0;

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
            VarBuffers& buffers = m_varBuffers[i];

            // Creates the uniform buffer for view data
            if(!CreateBuffer(m_allocator, buffers.viewDataBuffer.buffer, 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
                sizeof(BlitzenEngine::CameraViewData), VMA_ALLOCATION_CREATE_MAPPED_BIT
            ))
                return 0;

            buffers.viewDataBuffer.pData = reinterpret_cast<BlitzenEngine::CameraViewData*>(
                buffers.viewDataBuffer.buffer.allocation->GetMappedData()
            );
            
            WriteBufferDescriptorSets(buffers.viewDataBuffer.descriptorWrite, 
                buffers.viewDataBuffer.bufferInfo, buffers.viewDataBuffer.descriptorType, 
                buffers.viewDataBuffer.descriptorBinding, buffers.viewDataBuffer.buffer.bufferHandle
            );

			VkDeviceSize transformBufferSize = sizeof(BlitzenEngine::MeshTransform) * transforms.GetSize();
            if(!SetupPushDescriptorBuffer(m_device, m_allocator,
                buffers.transformBuffer, buffers.transformStagingBuffer,
                transformBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, transforms.Data()
            ))
                return 0;

			buffers.pTransformData = reinterpret_cast<BlitzenEngine::MeshTransform*>(
				buffers.transformStagingBuffer.allocationInfo.pMappedData
			);

			BeginCommandBuffer(m_frameToolsList[i].commandBuffer, 
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
            );

			CopyBufferToBuffer(m_frameToolsList[i].commandBuffer,
				buffers.transformStagingBuffer.bufferHandle,
				buffers.transformBuffer.buffer.bufferHandle,
				transformBufferSize, 0, 0
			);

			SubmitCommandBuffer(m_graphicsQueue.handle, m_frameToolsList[i].commandBuffer);
            vkQueueWaitIdle(m_graphicsQueue.handle);
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

        const BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices = 
            pResources->GetVerticesArray();
        const BlitCL::DynamicArray<uint32_t>& indices = pResources->GetIndicesArray();

        BlitzenEngine::RenderObject* pRenderObjects = pResources->renders;
        uint32_t renderObjectCount = pResources->renderObjectCount;

        const BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces = 
            pResources->GetSurfaceArray();

        const BlitzenEngine::Material* pMaterials = pResources->GetMaterialArrayPointer();
        uint32_t materialCount = pResources->GetMaterialCount();

        const BlitCL::DynamicArray<BlitzenEngine::Meshlet>& meshlets = 
            pResources->GetClusterArray();
        const BlitCL::DynamicArray<uint32_t>& meshletData = pResources->GetClusterIndices();

        BlitzenEngine::RenderObject* pOnpcRenderObjects = pResources->onpcReflectiveRenderObjects;
        uint32_t onpcRenderObjectCount = pResources->onpcReflectiveRenderObjectCount;

		BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& transforms = pResources->transforms;

        // When raytracing is active, some additional flags are needed for the vertex and index buffer
        uint32_t geometryBuffersRaytracingFlags = m_stats.bRayTracingSupported ?
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR 
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        : 0;

        // Global vertex buffer
        VkDeviceSize vertexBufferSize = sizeof(BlitzenEngine::Vertex) * vertices.GetSize();
        if(vertexBufferSize == 0)
            return 0;
        AllocatedBuffer stagingVertexBuffer;
        if(!SetupPushDescriptorBuffer(
            m_device, m_allocator, 
            m_currentStaticBuffers.vertexBuffer, stagingVertexBuffer, 
            vertexBufferSize, 
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | // Primary flags
            geometryBuffersRaytracingFlags, // Raytracing flags
            vertices.Data() // buffer data
        ))
            return 0;

        // Global index buffer
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.GetSize();
        if(indexBufferSize == 0)
            return 0;
        AllocatedBuffer stagingIndexBuffer;
        CreateStorageBufferWithStagingBuffer(
            m_allocator, m_device, 
            indices.Data(), // Buffer data
            m_currentStaticBuffers.indexBuffer, stagingIndexBuffer, 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | // Primary flags 
            geometryBuffersRaytracingFlags, // Raytracing flags
            indexBufferSize
        );
        if(m_currentStaticBuffers.indexBuffer.bufferHandle == VK_NULL_HANDLE)
            return 0;

        // Standard render object buffer
        VkDeviceSize renderObjectBufferSize = sizeof(BlitzenEngine::RenderObject) * renderObjectCount;
        if(renderObjectBufferSize == 0)
            return 0;
        AllocatedBuffer renderObjectStagingBuffer;
        if(!SetupPushDescriptorBuffer(
            m_device, m_allocator, 
            m_currentStaticBuffers.renderObjectBuffer, renderObjectStagingBuffer, 
            renderObjectBufferSize, 
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            pRenderObjects // buffer data
        ))
            return 0;

        // Render object buffer that holds objects that use Oblique near plane clipping
        VkDeviceSize onpcRenderObjectBufferSize = 
        sizeof(BlitzenEngine::RenderObject) * onpcRenderObjectCount;
        AllocatedBuffer onpcRenderObjectStagingBuffer;
        if(onpcRenderObjectBufferSize != 0)
            if(!SetupPushDescriptorBuffer(
                m_device, m_allocator, 
                m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer, 
                onpcRenderObjectStagingBuffer,
                onpcRenderObjectBufferSize, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                pOnpcRenderObjects // buffer data
            ))
                return 0;

        // Buffer that holds primitives (surfaces that can be drawn)
        VkDeviceSize surfaceBufferSize = sizeof(BlitzenEngine::PrimitiveSurface) * surfaces.GetSize();
        if(surfaceBufferSize == 0)
            return 0;
        AllocatedBuffer surfaceStagingBuffer;
        if(!SetupPushDescriptorBuffer(
            m_device, m_allocator, 
            m_currentStaticBuffers.surfaceBuffer, surfaceStagingBuffer, 
            surfaceBufferSize, 
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            surfaces.Data() // buffer data
        ))
            return 0;

        // Global material buffer
        VkDeviceSize materialBufferSize = sizeof(BlitzenEngine::Material) * materialCount;
        if(materialBufferSize == 0)
            return 0;
        AllocatedBuffer materialStagingBuffer; 
        if(!SetupPushDescriptorBuffer(
            m_device, m_allocator, 
            m_currentStaticBuffers.materialBuffer, materialStagingBuffer, 
            materialBufferSize, 
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            const_cast<BlitzenEngine::Material*>(pMaterials) // buffer data
        ))
            return 0;

        // Indirect draw buffer (VkDrawIndexedIndirectCommand holder and render object id)
        VkDeviceSize indirectDrawBufferSize = sizeof(IndirectDrawData) * renderObjectCount;
        if(indirectDrawBufferSize == 0)
            return 0;
        if(!SetupPushDescriptorBuffer(
            m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, 
            m_currentStaticBuffers.indirectDrawBuffer, indirectDrawBufferSize, 
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
        ))
            return 0;

        
        // Mesh shader indirect commands
        VkDeviceSize indirectTaskBufferSize = sizeof(IndirectTaskData) * renderObjectCount;

        // Cluster data
        VkDeviceSize meshletBufferSize = sizeof(BlitzenEngine::Meshlet) * meshlets.GetSize();
        AllocatedBuffer meshletStagingBuffer;
        VkDeviceSize meshletDataBufferSize = sizeof(uint32_t) * meshletData.GetSize();
        AllocatedBuffer meshletDataStagingBuffer;

        if(m_stats.meshShaderSupport)
        {
            // Indirect task buffer
            if(indirectTaskBufferSize == 0)
                return 0;
            if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, 
                m_currentStaticBuffers.indirectTaskBuffer, indirectTaskBufferSize, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
            ))
                return 0;

            /*
                TODO: Meshlet data should be available for cluster mode as well, not just mesh shaders
            */

            // Meshlets / Clusters
            if(meshletBufferSize == 0)
                return 0;
            if(!SetupPushDescriptorBuffer(m_device, m_allocator, 
                m_currentStaticBuffers.meshletBuffer, meshletStagingBuffer, 
                meshletBufferSize, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                meshlets.Data()
            ))
                return 0;

            // Cluster indices
            if(meshletDataBufferSize == 0)
                return 0;
            if(!SetupPushDescriptorBuffer(m_device, m_allocator, 
                m_currentStaticBuffers.meshletDataBuffer, meshletDataStagingBuffer, 
                meshletDataBufferSize, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                meshletData.Data()
            ))
                return 0;
        }

        // Indirect count buffer
        if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, 
            m_currentStaticBuffers.indirectCountBuffer, 
            sizeof(uint32_t), // holds a single integer, manipulated by culling shaders
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | // draw command treats it as indirect buffer
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | // data transfer with vkCmdBufferFill, initializing to 0
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT // culling shaders treat it as write storage buffer
        ))
            return 0;

        // Visiblity buffer for every render object (could be integrated to the render object as 8bit integer)
        VkDeviceSize visibilityBufferSize = sizeof(uint32_t) * renderObjectCount;
        if(visibilityBufferSize == 0)
            return 0;
        if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, 
            m_currentStaticBuffers.visibilityBuffer, visibilityBufferSize, 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        ))
            return 0;

        /*
            Buffers ready. Data transfer -for the buffers that need it- can commence
        */

        // Start recording the transfer commands
        VkCommandBuffer& commandBuffer = m_frameToolsList[0].commandBuffer;
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // Vertex data copy
        CopyBufferToBuffer(commandBuffer, 
            stagingVertexBuffer.bufferHandle, // Temporary staging buffer
            m_currentStaticBuffers.vertexBuffer.buffer.bufferHandle, // GPU only buffer
            vertexBufferSize, 
            0, 0 // offsets
        );

        // Index data copy
        CopyBufferToBuffer(commandBuffer, 
            stagingIndexBuffer.bufferHandle, // temporary staging buffer
            m_currentStaticBuffers.indexBuffer.bufferHandle, // GPU only buffer
            indexBufferSize, 
            0, 0 // offsets
        );

        // Render objects
        CopyBufferToBuffer(commandBuffer, 
            renderObjectStagingBuffer.bufferHandle, // temporary staging buffer
            m_currentStaticBuffers.renderObjectBuffer.buffer.bufferHandle, // GPU only buffer
            renderObjectBufferSize, 
            0, 0 // offsets
        );

        // Render objects that use Oblique Near-Plane Clipping
        if(onpcRenderObjectBufferSize != 0)
            CopyBufferToBuffer(commandBuffer, 
                onpcRenderObjectStagingBuffer.bufferHandle, // temporary staging buffer
                m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.buffer.bufferHandle, // GPU only buffer
                onpcRenderObjectBufferSize, 
                0, 0 // offsets
            );

        // Primitive surfaces
        CopyBufferToBuffer(commandBuffer, 
            surfaceStagingBuffer.bufferHandle, // temporary staging buffer
            m_currentStaticBuffers.surfaceBuffer.buffer.bufferHandle, // GPU only buffer
            surfaceBufferSize, 
            0, 0 // offsets
        );

        // Materials
        CopyBufferToBuffer(commandBuffer, 
            materialStagingBuffer.bufferHandle, // temporary staging buffer
            m_currentStaticBuffers.materialBuffer.buffer.bufferHandle, // GPU only buffer
            materialBufferSize, 
            0, 0 // offsets
        );
        
        if(m_stats.meshShaderSupport)
        {
            // Copies the cluster data held by the staging buffer to the meshlet buffer
            CopyBufferToBuffer(commandBuffer, meshletStagingBuffer.bufferHandle, 
            m_currentStaticBuffers.meshletBuffer.buffer.bufferHandle, meshletBufferSize, 
            0, 0);

            CopyBufferToBuffer(commandBuffer, meshletDataStagingBuffer.bufferHandle, 
            m_currentStaticBuffers.meshletDataBuffer.buffer.bufferHandle, meshletDataBufferSize, 
            0, 0);
        }

        // Visibility buffer will start with only zeroes. The first frame will do noting, but that should be fine
        vkCmdFillBuffer(commandBuffer, m_currentStaticBuffers.visibilityBuffer.buffer.bufferHandle, 
            0, visibilityBufferSize, 0
        );
        
        // Submit the commands and wait for the queue to finish
        SubmitCommandBuffer(m_graphicsQueue.handle, commandBuffer);
        vkQueueWaitIdle(m_graphicsQueue.handle);

        // Sets up raytracing acceleration structures, if it is requested and supported
        if(m_stats.bRayTracingSupported)
        {
            if(!BuildBlas(surfaces, pResources->GetPrimitiveVertexCounts()))
                return 0;
            if(!BuildTlas(pRenderObjects, renderObjectCount, transforms.Data(), surfaces.Data()))
                return 0;
        }

        // Fails if there are no textures to load
        if(textureCount == 0)
            return 0;

        // The descriptor will have multiple descriptors of combined image sampler type. The count is derived from the amount of textures loaded
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = static_cast<uint32_t>(textureCount);

        // Creates the descriptor pool for the textures
        m_textureDescriptorPool.handle = CreateDescriptorPool(m_device, 1, &poolSize, 
        1);
        if(m_textureDescriptorPool.handle == VK_NULL_HANDLE)
            return 0;
 
        // Allocates the descriptor set that will be used to bind the textures
        if(!AllocateDescriptorSets(m_device, m_textureDescriptorPool.handle, 
        &m_textureDescriptorSetlayout.handle, 1, &m_textureDescriptorSet))
            return 0;

        // Create image infos for every texture to be passed to the VkWriteDescriptorSet
        BlitCL::DynamicArray<VkDescriptorImageInfo> imageInfos(textureCount);
        for(size_t i = 0; i < imageInfos.GetSize(); ++i)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = loadedTextures[i].image.imageView;
            imageInfos[i].sampler = loadedTextures[i].sampler;
        }

        // Update every descriptor set so that it is available at draw time
        VkWriteDescriptorSet write{};
        WriteImageDescriptorSets(write, imageInfos.Data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_textureDescriptorSet, 
        static_cast<uint32_t>(imageInfos.GetSize()), 0);
        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

        return 1;
    }

    static VkResult CreateAccelerationStructureKHR(VkInstance instance, VkDevice device, 
    const VkAccelerationStructureCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator,
    VkAccelerationStructureKHR* pAccelerationStructure)
    {
        auto func = (PFN_vkCreateAccelerationStructureKHR) vkGetInstanceProcAddr(
        instance, "vkCreateAccelerationStructureKHR");
        if (func != nullptr) 
        {
            return func(device, pCreateInfo, pAllocator, pAccelerationStructure);
        }

        return VK_NOT_READY;
    }
    
    static void BuildAccelerationStructureKHR(VkInstance instance, VkCommandBuffer commandBuffer, uint32_t infoCount, 
    const VkAccelerationStructureBuildGeometryInfoKHR* buildInfos, const VkAccelerationStructureBuildRangeInfoKHR* const *ppRangeInfos)
    {
        auto func = (PFN_vkCmdBuildAccelerationStructuresKHR) vkGetInstanceProcAddr(
        instance, "vkCmdBuildAccelerationStructuresKHR");
        if(func != nullptr)
        {
            func(commandBuffer, infoCount, buildInfos, ppRangeInfos);
        }
    }
    
    static void GetAccelerationStructureBuildSizesKHR(VkInstance instance, VkDevice device, 
    VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, 
    const uint32_t* pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR* pBuildSizes)
    {
        auto func = (PFN_vkGetAccelerationStructureBuildSizesKHR) vkGetInstanceProcAddr(
        instance, "vkGetAccelerationStructureBuildSizesKHR");
        if(func != nullptr)
        {
            func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pBuildSizes);
        }
    }

    static VkDeviceAddress GetAccelerationStructureDeviceAddressKHR(VkInstance instance, VkDevice device, 
    const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
    {
        auto func = (PFN_vkGetAccelerationStructureDeviceAddressKHR) vkGetInstanceProcAddr(
        instance, "vkGetAccelerationStructureDeviceAddressKHR");
        if(func != nullptr)
        {
            return func(device, pInfo);
        }

        return (VkDeviceAddress)VK_NULL_HANDLE;
    }

    uint8_t VulkanRenderer::BuildBlas(const BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces, 
        const BlitCL::DynamicArray<uint32_t>& primitiveVertexCounts)
    {
        if(!surfaces.GetSize())
            return 0;

        BlitCL::DynamicArray<uint32_t> primitiveCounts(surfaces.GetSize());
        BlitCL::DynamicArray<VkAccelerationStructureGeometryKHR> geometries(surfaces.GetSize(), {});
	    BlitCL::DynamicArray<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos(surfaces.GetSize(), {});

        BlitCL::DynamicArray<size_t> accelerationOffsets(surfaces.GetSize());
	    BlitCL::DynamicArray<size_t> accelerationSizes(surfaces.GetSize());
	    BlitCL::DynamicArray<size_t> scratchOffsets(surfaces.GetSize());

        const size_t ce_alignment = 256; // required by spec for acceleration structures

        size_t totalAccelerationSize = 0;
        size_t totalScratchSize = 0;

        // Gets the address of the vertex buffer and then the index buffer
        VkDeviceAddress vertexBufferAddress = GetBufferAddress(m_device, 
        m_currentStaticBuffers.vertexBuffer.buffer.bufferHandle);
	    VkDeviceAddress indexBufferAddress = GetBufferAddress(m_device, 
        m_currentStaticBuffers.indexBuffer.bufferHandle);

        for(size_t i = 0; i < surfaces.GetSize(); ++i)
        {
            const auto& surface = surfaces[i];
            VkAccelerationStructureGeometryKHR& geometry = geometries[i];// The as geometry that will be initialized in this loop
            VkAccelerationStructureBuildGeometryInfoKHR& buildInfo = buildInfos[i];

            // Specifies the geometry for this accelration structure
            geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.pNext = nullptr;
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;

            geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.pNext = nullptr;

            // Passing vertex data
            geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            // Gets the precise address of the vertex buffer for the current surface (needs to be incremented by the vertex offset)
            geometry.geometry.triangles.vertexData.deviceAddress = 
            static_cast<VkDeviceAddress>(vertexBufferAddress + surface.vertexOffset * sizeof(BlitzenEngine::Vertex));
            geometry.geometry.triangles.vertexStride = sizeof(BlitzenEngine::Vertex);
            // Primitive vertex count (at the moment), is an array created for this one, optinal line of code
            geometry.geometry.triangles.maxVertex = primitiveVertexCounts[i];

            // Passing index data
            geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            // Precise address of the index buffer
            geometry.geometry.triangles.indexData.deviceAddress = 
            static_cast<VkDeviceAddress>(indexBufferAddress + surface.meshLod[0].firstIndex * sizeof(uint32_t));
            

            // Build info for the acceleration structu. Takes the geometry struct from above and some other configs
            buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buildInfo.pNext = nullptr;
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;// Bottom level since the as is for geometries
            buildInfo.flags = 
                VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | 
                VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
            buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.geometryCount = 1;
            buildInfo.pGeometries = &geometry;


            primitiveCounts[i] = surface.meshLod[0].indexCount / 3;

            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
            sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		    GetAccelerationStructureBuildSizesKHR(m_instance, m_device, 
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCounts[i], &sizeInfo);

    		accelerationOffsets[i] = totalAccelerationSize;
            accelerationSizes[i] = sizeInfo.accelerationStructureSize;
            scratchOffsets[i] = totalScratchSize;

		    totalAccelerationSize = (totalAccelerationSize + sizeInfo.accelerationStructureSize + 
            ce_alignment - 1) & ~(ce_alignment - 1);
            totalScratchSize = (totalScratchSize + sizeInfo.buildScratchSize + ce_alignment - 1) & ~(ce_alignment - 1);
        }

        if(!CreateBuffer(m_allocator, m_currentStaticBuffers.blasBuffer, 
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, totalAccelerationSize, 0))
            return 0;

        AllocatedBuffer stagingBuffer;
        if(!CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, totalScratchSize, 0))
            return 0;

        VkDeviceAddress blasStagingBufferAddress = GetBufferAddress(m_device, stagingBuffer.bufferHandle);

        m_currentStaticBuffers.blasData.Resize(surfaces.GetSize());

        // Need to empty-brace initialize every damned struct otherwise Vulkan breaks
        BlitCL::DynamicArray<VkAccelerationStructureBuildRangeInfoKHR> buildRanges(surfaces.GetSize(), {});
	    BlitCL::DynamicArray<const VkAccelerationStructureBuildRangeInfoKHR*> buildRangePtrs(surfaces.GetSize());

        for (size_t i = 0; i < surfaces.GetSize(); ++i)
	    {
		    VkAccelerationStructureCreateInfoKHR accelerationInfo {};
            accelerationInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		    accelerationInfo.buffer = m_currentStaticBuffers.blasBuffer.bufferHandle;
		    accelerationInfo.offset = accelerationOffsets[i];
		    accelerationInfo.size = accelerationSizes[i];
		    accelerationInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

		    if(CreateAccelerationStructureKHR(m_instance, m_device, &accelerationInfo, nullptr, 
            &m_currentStaticBuffers.blasData[i].handle) != VK_SUCCESS)
                return 0;

		    buildInfos[i].dstAccelerationStructure = m_currentStaticBuffers.blasData[i].handle;
		    buildInfos[i].scratchData.deviceAddress = blasStagingBufferAddress + scratchOffsets[i];

            VkAccelerationStructureBuildRangeInfoKHR& buildRange = buildRanges[i];
		    buildRanges[i].primitiveCount = primitiveCounts[i];
		    buildRangePtrs[i] = &buildRanges[i];
        }

        VkCommandBuffer commandBuffer = m_frameToolsList[0].commandBuffer;
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        BuildAccelerationStructureKHR(m_instance, commandBuffer, static_cast<uint32_t>(surfaces.GetSize()), 
        buildInfos.Data(), buildRangePtrs.Data());
        SubmitCommandBuffer(m_computeQueue.handle, commandBuffer);
        vkQueueWaitIdle(m_computeQueue.handle);

        return 1;
    }

    uint8_t VulkanRenderer::BuildTlas(BlitzenEngine::RenderObject* pDraws, uint32_t drawCount, 
        BlitzenEngine::MeshTransform* pTransforms, BlitzenEngine::PrimitiveSurface* pSurfaces
    )
    {
        // Retrieves the device address of each acceleration structure that was build earlier
        BlitCL::DynamicArray<VkDeviceAddress> blasAddresses{m_currentStaticBuffers.blasData.GetSize()};
        for(size_t i = 0; i < blasAddresses.GetSize(); ++i)
        {
            VkAccelerationStructureDeviceAddressInfoKHR	addressInfo{};
            addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
            addressInfo.accelerationStructure = m_currentStaticBuffers.blasData[i].handle;
            blasAddresses[i] = GetAccelerationStructureDeviceAddressKHR(m_instance, m_device, &addressInfo);
        }

        // Creates an object buffer that will hold a VkAccelerationsStructureInstanceKHR for each object loaded
        AllocatedBuffer objectBuffer;
        if(!CreateBuffer(m_allocator, objectBuffer, 
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(VkAccelerationStructureInstanceKHR) * drawCount, VMA_ALLOCATION_CREATE_MAPPED_BIT))
            return 0;

        // Creates a VkAccelrationStructureInstanceKHR for each instance's transform and copies it to the object buffer
        for(size_t i = 0; i < drawCount; ++i)
        {
            const auto& object = pDraws[i];
            const auto& transform = pTransforms[object.transformId];
            const auto& surface = pSurfaces[object.surfaceId];

            // Casts the orientation quat to a matrix
            auto orientationTranspose{BlitML::Transpose(BlitML::QuatToMat4(transform.orientation))};
            auto xform =  orientationTranspose * transform.scale;

            VkAccelerationStructureInstanceKHR instance{};
            // Copies the first 3 elements of the 1st row of the matrix to the 1st row of the Vulkan side matrix
            BlitzenCore::BlitMemCopy(instance.transform.matrix[0], &xform[0], sizeof(float) * 3);
            // Copies the first 3 elements of the 2nd row of the matrix to the 2nd row of the Vulkan side matrix
            BlitzenCore::BlitMemCopy(instance.transform.matrix[1], &xform[4], sizeof(float) * 3);
            // Copies the first 3 elements of the 3rd row of the matrix to the 3rd row of the Vulkan side matrix
            BlitzenCore::BlitMemCopy(instance.transform.matrix[2], &xform[8], sizeof(float) * 3);

            instance.transform.matrix[0][3] = transform.pos.x;
            instance.transform.matrix[1][3] = transform.pos.y;
            instance.transform.matrix[2][3] = transform.pos.z;

            instance.instanceCustomIndex = i;
            instance.mask = 1 << surface.postPass;
            instance.flags = surface.postPass ? VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR : 0;
            instance.accelerationStructureReference = blasAddresses[object.surfaceId];

            auto pData = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(
            objectBuffer.allocation->GetMappedData()) + i;
            BlitzenCore::BlitMemCopy(pData, &instance, sizeof(VkAccelerationStructureInstanceKHR));
        }

        VkAccelerationStructureGeometryKHR geometry{}; 
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.data.deviceAddress = GetBufferAddress(m_device, objectBuffer.bufferHandle);

        // Initial values for build info, more data will become available later in the function
	    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;// Top level since the as is for instances
	    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // Could have different modes for this
	    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	    buildInfo.geometryCount = 1;
	    buildInfo.pGeometries = &geometry;

        // Gets the size
	    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{}; 
        sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        // Gets the acceleration structure build sizes
	    GetAccelerationStructureBuildSizesKHR(m_instance, m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, 
        &buildInfo, &drawCount, &sizeInfo);

        // Creates the Tlas buffer based on the build size that was retrieved above
        // For raytracing, the below struct needs to be added to the pNext chain of the descriptor set layout
	    if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, 
            m_currentStaticBuffers.tlasBuffer, sizeInfo.accelerationStructureSize, 
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
        ))
            return 0;

	    AllocatedBuffer stagingBuffer;
	    if(!CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, sizeInfo.buildScratchSize, 0))
            return 0;

        // Creates the accleration structure
        VkAccelerationStructureCreateInfoKHR accelerationInfo{}; 
        accelerationInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationInfo.buffer = m_currentStaticBuffers.tlasBuffer.buffer.bufferHandle;
        accelerationInfo.size = sizeInfo.accelerationStructureSize;
        accelerationInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        if(CreateAccelerationStructureKHR(m_instance, m_device, &accelerationInfo, 
        nullptr, &m_currentStaticBuffers.tlasData.handle) != VK_SUCCESS)
            return 0;

        buildInfo.dstAccelerationStructure = m_currentStaticBuffers.tlasData.handle;
        buildInfo.scratchData.deviceAddress = GetBufferAddress(m_device, stagingBuffer.bufferHandle);
        
        VkAccelerationStructureBuildRangeInfoKHR buildRange = {};
        buildRange.primitiveCount = drawCount;
        const VkAccelerationStructureBuildRangeInfoKHR* pBuildRange = &buildRange;

        VkCommandBuffer commandBuffer = m_frameToolsList[0].commandBuffer;
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        BuildAccelerationStructureKHR(m_instance, commandBuffer, 1, &buildInfo, &pBuildRange);
        SubmitCommandBuffer(m_graphicsQueue.handle, commandBuffer);
        vkQueueWaitIdle(m_graphicsQueue.handle);

        return 1;
    }

    void VulkanRenderer::UpdateBuffers(BlitzenEngine::RenderingResources* pResources, 
        FrameTools& tools, VarBuffers& buffers)
    {
		VkDeviceSize transformDataSize = sizeof(BlitzenEngine::MeshTransform) * pResources->transforms.GetSize();

        BeginCommandBuffer(tools.transferCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        CopyBufferToBuffer(tools.transferCommandBuffer, buffers.transformStagingBuffer.bufferHandle,
            buffers.transformBuffer.buffer.bufferHandle, transformDataSize, 0, 0
        );

        VkSemaphoreSubmitInfo bufferCopySemaphoreInfo{};
        CreateSemahoreSubmitInfo(bufferCopySemaphoreInfo, tools.buffersReadySemaphore.handle,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
        );
        SubmitCommandBuffer(m_transferQueue.handle, tools.transferCommandBuffer, 
            0, nullptr, 1, &bufferCopySemaphoreInfo
        );
    }
}