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

    uint8_t VulkanRenderer::RenderingAttachmentsInit()
    {
        /*
            Color attachment handle and value configuration
        */
        // Color attachment sammpler will be used to copy to the swapchain image
        if (!m_colorAttachment.SamplerInit(m_device, VK_FILTER_NEAREST,
            VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, nullptr
        ))
        {
            BLIT_ERROR("Failed to create color attachment sampler")
                return 0;
        }
        // Create depth attachment image and image view
        if (!CreatePushDescriptorImage(m_device, m_allocator, m_colorAttachment,
            { m_drawExtent.width, m_drawExtent.height, 1 }, // extent
            ce_colorAttachmentFormat, ce_colorAttachmentImageUsage,
            1, VMA_MEMORY_USAGE_GPU_ONLY
        ))
        {
            BLIT_ERROR("Failed to create color attachment image resource")
                return 0;
        }
        // Rendering attachment info, most values stay constant
        CreateRenderingAttachmentInfo(m_colorAttachmentInfo, m_colorAttachment.image.imageView,
            ce_ColorAttachmentLayout, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
            ce_WindowClearColor);
        /*
            Depth attachement value and handle configuration
        */
        // Creates a sampler for the depth attachment, used for the depth pyramid
        VkSamplerReductionModeCreateInfo reductionInfo{};
        reductionInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reductionInfo.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
        if (!m_depthAttachment.SamplerInit(m_device,
            VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, &reductionInfo
        ))
        {
            BLIT_ERROR("Failed to create depth attachment sampler")
                return 0;
        }
        // Creates rendering attachment image resource for depth attachment
        if (!CreatePushDescriptorImage(m_device, m_allocator, m_depthAttachment,
            { m_drawExtent.width, m_drawExtent.height, 1 },
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
        if (!CreateDepthPyramid(m_depthPyramid, m_depthPyramidExtent,
            m_depthPyramidMips, m_depthPyramidMipLevels,
            m_drawExtent, m_device, m_allocator))
        {
            BLIT_ERROR("Failed to create the depth pyramid")
                return 0;
        }

        // Success
        return 1;
    }

    uint8_t VulkanRenderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources, 
        float& pyramidWidth, float& pyramidHeight)
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

        if (!RenderingAttachmentsInit())
        {
            BLIT_ERROR("Failed to create rendering attachments");
            return 0;
        }

        if(!CreateDescriptorLayouts())
        {
            BLIT_ERROR("Failed to create descriptor set layouts")
            return 0;
        }

        if (!CreatePipelineLayouts())
        {
            BLIT_ERROR("Failed to create pipeline layouts");
            return 0;
        }

        if(!VarBuffersInit(pResources->transforms))
        {
            BLIT_ERROR("Failed to create uniform buffers")
            return 0;
        }

        if(!StaticBuffersInit(pResources))
        {
            BLIT_ERROR("Failed to upload data to the GPU")
            return 0;
        }

        if (!CreateComputeShaders())
        {
            BLIT_ERROR("Failed to create compute shaders");
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
        commandPoolsInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
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
            auto& frameTools{ m_frameToolsList[i] };

            // Main command buffer
            if (vkCreateCommandPool(m_device, &commandPoolsInfo,
                m_pCustomAllocator, &frameTools.mainCommandPool.handle) != VK_SUCCESS)
            {
                return 0;
            }
            commandBuffersInfo.commandPool = frameTools.mainCommandPool.handle;
            if (vkAllocateCommandBuffers(m_device, &commandBuffersInfo,
                &frameTools.commandBuffer) != VK_SUCCESS)
            {
                return 0;
            }

            // Dedicated transfer command buffer
            if (vkCreateCommandPool(m_device, &dedicatedCommandPoolsInfo,
                nullptr, &frameTools.transferCommandPool.handle) != VK_SUCCESS)
            {
                return 0;
            }
			dedicatedCmbInfo.commandPool = frameTools.transferCommandPool.handle;
            if (vkAllocateCommandBuffers(m_device, &dedicatedCmbInfo,
                &frameTools.transferCommandBuffer) != VK_SUCCESS)
            {
                return 0;
            }

            if (vkCreateFence(m_device, &fenceInfo, m_pCustomAllocator,
                &frameTools.inFlightFence.handle) != VK_SUCCESS)
            {
                return 0;
            }

            if (vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator,
                &frameTools.imageAcquiredSemaphore.handle) != VK_SUCCESS)
            {
                return 0;
            }

            // Semaphore that blocks presentation before the commands are all executed
            if (vkCreateSemaphore(m_device, &semaphoresInfo,m_pCustomAllocator, 
                &frameTools.readyToPresentSemaphore.handle) != VK_SUCCESS)
            {
                return 0;
            }

            // Semaphore to wait for buffer to be updated
            if (vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator,
                &frameTools.buffersReadySemaphore.handle) != VK_SUCCESS)
            {
                return 0;
            }
        }

        return 1;
    }

    uint8_t CreateDepthPyramid(PushDescriptorImage& depthPyramidImage, VkExtent2D& depthPyramidExtent, 
        VkImageView* depthPyramidMips, uint8_t& depthPyramidMipLevels, 
        VkExtent2D drawExtent, VkDevice device, VmaAllocator allocator
    )
    {
		const VkFormat depthPyramidFormat = VK_FORMAT_R32_SFLOAT;
        const VkImageUsageFlags depthPyramidUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT; 

        // Conservative starting extent
        depthPyramidExtent.width = BlitML::PreviousPow2(drawExtent.width);
        depthPyramidExtent.height = BlitML::PreviousPow2(drawExtent.height);
		depthPyramidMipLevels = 
            BlitML::GetDepthPyramidMipLevels(depthPyramidExtent.width, depthPyramidExtent.height);
    
        // Creates the primary depth pyramid image
        if (!CreatePushDescriptorImage(device, allocator, depthPyramidImage,
            { depthPyramidExtent.width, depthPyramidExtent.height, 1 },
            depthPyramidFormat, depthPyramidUsage, depthPyramidMipLevels,
            VMA_MEMORY_USAGE_GPU_ONLY))
        {
            return 0;
        }
    
        // Create the mip levels of the depth pyramid
        for(uint8_t i = 0; i < depthPyramidMipLevels; ++i)
        {
            if (!CreateImageView(device, depthPyramidMips[size_t(i)],
                depthPyramidImage.image.image, depthPyramidFormat, i, 1))
            {
                return 0;
            }
        }
    
        return 1;
    }

    VkDescriptorSetLayout VulkanRenderer::CreateGPUBufferPushDescriptorBindings(
        VkDescriptorSetLayoutBinding* pBindings, uint32_t bindingCount)
    {
        constexpr uint32_t viewDataBindingID = 0;
        constexpr uint32_t vertexBindingID = 1;
        constexpr uint32_t depthImageBindingID = 2;
        constexpr uint32_t renderObjectsBindingID = 3;
        constexpr uint32_t transformsBindingID = 4;
        constexpr uint32_t materialsBindingID = 5;
        constexpr uint32_t indirectCommandsBindingID = 6;
        constexpr uint32_t indirectCountBindingID = 7;
        constexpr uint32_t objectVisibilitiesBindingID = 8;
        constexpr uint32_t primitivesBindingID = 9;
        constexpr uint32_t clustersBindingID = 10;
        constexpr uint32_t clusterIndicesBindingID = 11;
        constexpr uint32_t onpcObjectsBindingID = 12;
        constexpr uint32_t tlasBindingID = 13;
        constexpr uint32_t indirectTaskCommandsBindingID = 14;
        
        // Every binding in the pushDescriptorSetLayout will have one descriptor
        constexpr uint32_t descriptorCountOfEachPushDescriptorLayoutBinding = 1;

        auto viewDataShaderStageFlags = m_stats.meshShaderSupport ?
            VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT :
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[viewDataBindingID],
            m_varBuffers[0].viewDataBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_varBuffers[0].viewDataBuffer.descriptorType,
            viewDataShaderStageFlags);

        auto vertexBufferShaderStageFlags = m_stats.meshShaderSupport ?
            VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[vertexBindingID],
            m_currentStaticBuffers.vertexBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.vertexBuffer.descriptorType,
            vertexBufferShaderStageFlags);

        auto surfaceBufferShaderStageFlags = m_stats.meshShaderSupport ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[primitivesBindingID],
            m_currentStaticBuffers.surfaceBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.surfaceBuffer.descriptorType,
            surfaceBufferShaderStageFlags);

        auto clusterBufferShaderStageFlags = m_stats.meshShaderSupport ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[clustersBindingID],
            m_currentStaticBuffers.meshletBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.meshletBuffer.descriptorType,
            clusterBufferShaderStageFlags);

        auto clusterDataBufferShaderStageFlags = m_stats.meshShaderSupport ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[clusterIndicesBindingID],
            m_currentStaticBuffers.meshletDataBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.meshletDataBuffer.descriptorType,
            clusterDataBufferShaderStageFlags);

        if (m_stats.meshShaderSupport)
        {
            CreateDescriptorSetLayoutBinding(pBindings[indirectTaskCommandsBindingID],
                m_currentStaticBuffers.indirectTaskBuffer.descriptorBinding,
                descriptorCountOfEachPushDescriptorLayoutBinding,
                m_currentStaticBuffers.indirectTaskBuffer.descriptorType,
                VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT);
        }

        CreateDescriptorSetLayoutBinding(pBindings[depthImageBindingID], 
            Ce_DepthPyramidImageBindingID, descriptorCountOfEachPushDescriptorLayoutBinding,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[renderObjectsBindingID],
            m_currentStaticBuffers.renderObjectBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.renderObjectBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[transformsBindingID],
            m_varBuffers[0].transformBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_varBuffers[0].transformBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[materialsBindingID],
            m_currentStaticBuffers.materialBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.materialBuffer.descriptorType,
            VK_SHADER_STAGE_FRAGMENT_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[indirectCommandsBindingID],
            m_currentStaticBuffers.indirectDrawBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.indirectDrawBuffer.descriptorType,
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[indirectCountBindingID],
            m_currentStaticBuffers.indirectCountBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.indirectCountBuffer.descriptorType,
            VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[objectVisibilitiesBindingID],
            m_currentStaticBuffers.visibilityBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.visibilityBuffer.descriptorType,
            VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[onpcObjectsBindingID],
            m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[tlasBindingID],
            m_currentStaticBuffers.tlasBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            m_currentStaticBuffers.tlasBuffer.descriptorType,
            VK_SHADER_STAGE_FRAGMENT_BIT);

        return CreateDescriptorSetLayout(m_device,bindingCount, pBindings, 
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
    }

    uint8_t VulkanRenderer::CreateDescriptorLayouts()
    {
        constexpr uint32_t descriptorCountOfEachPushDescriptorLayoutBinding = 1;
        
        // The big GPU push descriptor set layout. Holds most buffers
        BlitCL::StaticArray<VkDescriptorSetLayoutBinding, 15> gpuPushDescriptorBindings{ {} };
        m_pushDescriptorBufferLayout.handle = CreateGPUBufferPushDescriptorBindings(
            gpuPushDescriptorBindings.Data(), m_stats.bRayTracingSupported ?
            (uint32_t)gpuPushDescriptorBindings.Size() - 1 : (uint32_t)gpuPushDescriptorBindings.Size() - 2);
        if (m_pushDescriptorBufferLayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create GPU buffer push descriptor layout");
            return 0;
        }

        // Descriptor set layout for textures
        VkDescriptorSetLayoutBinding texturesLayoutBinding{};
        CreateDescriptorSetLayoutBinding(texturesLayoutBinding, 0,static_cast<uint32_t>(textureCount), 
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_textureDescriptorSetlayout.handle = 
            CreateDescriptorSetLayout(m_device, 1, &texturesLayoutBinding);
        if (m_textureDescriptorSetlayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create texture descriptor set layout");
            return 0;
        }

        // Depth pyramid generation layout
        constexpr uint32_t Ce_DepthPyramidGenerationBindingCount = 2;
        BlitCL::StaticArray<VkDescriptorSetLayoutBinding, Ce_DepthPyramidGenerationBindingCount> 
            depthPyramidBindings{ {} };
        CreateDescriptorSetLayoutBinding(depthPyramidBindings[0], m_depthPyramid.m_descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, m_depthPyramid.m_descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT);
        CreateDescriptorSetLayoutBinding(depthPyramidBindings[1], m_depthAttachment.m_descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, m_depthAttachment.m_descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT);
        m_depthPyramidDescriptorLayout.handle = 
            CreateDescriptorSetLayout(m_device, Ce_DepthPyramidGenerationBindingCount, 
                depthPyramidBindings.Data(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if (m_depthPyramidDescriptorLayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create depth pyramid descriptor set layout")
            return 0;
        }

        // Generate presentation image layout
        constexpr uint32_t Ce_PresentationGenerationBindingCount = 2;
        BlitCL::StaticArray<VkDescriptorSetLayoutBinding, Ce_DepthPyramidGenerationBindingCount>
            presentGenerationBindings{ {} };
        CreateDescriptorSetLayoutBinding(presentGenerationBindings[0], 
            0, descriptorCountOfEachPushDescriptorLayoutBinding,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        CreateDescriptorSetLayoutBinding(presentGenerationBindings[1],
            m_colorAttachment.m_descriptorBinding, descriptorCountOfEachPushDescriptorLayoutBinding, 
            m_colorAttachment.m_descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);
        m_generatePresentationImageSetLayout.handle = CreateDescriptorSetLayout(m_device, 
            Ce_PresentationGenerationBindingCount, presentGenerationBindings.Data(),
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if (m_generatePresentationImageSetLayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create present image generation layout")
            return 0;
        }

        // Success 
        return 1;
    }

    uint8_t VulkanRenderer::CreatePipelineLayouts()
    {
        constexpr uint32_t Ce_SinglePointer = 1;

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
            BLIT_ARRAY_SIZE(defaultGraphicsPipelinesDescriptorSetLayouts), 
            defaultGraphicsPipelinesDescriptorSetLayouts, 
            Ce_SinglePointer, &globalShaderDataPushContant))
        {
            BLIT_ERROR("Failed to create main graphics pipeline layout");
            return 0;
        }

        // Culling shader shared layout (slight differences are not enough to create permutations
        VkPushConstantRange cullShaderPushConstant{};
        CreatePushConstantRange(cullShaderPushConstant,VK_SHADER_STAGE_COMPUTE_BIT, 
            sizeof(DrawCullShaderPushConstant));
        if (!CreatePipelineLayout(m_device, &m_drawCullLayout.handle,
            Ce_SinglePointer, &m_pushDescriptorBufferLayout.handle, // push descriptor set layout
            Ce_SinglePointer, &cullShaderPushConstant))
        {
            BLIT_ERROR("Failed to create culling pipeline layout");
            return 0;
        }

        // Layout for depth pyramid generation pipeline
        VkPushConstantRange depthPyramidMipExtentPushConstant{};
        CreatePushConstantRange(depthPyramidMipExtentPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, 
            sizeof(BlitML::vec2));
        if (!CreatePipelineLayout(m_device, &m_depthPyramidGenerationLayout.handle,
            Ce_SinglePointer, &m_depthPyramidDescriptorLayout.handle,
            Ce_SinglePointer, &depthPyramidMipExtentPushConstant))
        {
            BLIT_ERROR("Failed to create depth pyramid generation pipeline layout")
            return 0;
        }

        // I need to remove this one
        VkPushConstantRange onpcMatrixPushConstant{};
        CreatePushConstantRange(onpcMatrixPushConstant, VK_SHADER_STAGE_VERTEX_BIT, sizeof(BlitML::mat4));
        if (!CreatePipelineLayout(m_device, &m_onpcReflectiveGeometryLayout.handle,
            BLIT_ARRAY_SIZE(defaultGraphicsPipelinesDescriptorSetLayouts), 
            defaultGraphicsPipelinesDescriptorSetLayouts,
            Ce_SinglePointer, &onpcMatrixPushConstant))
        {
            BLIT_ERROR("Failed to create onpc graphics pipeline");
            return 0;
        }

        // Generate present image compute shader layout
        VkPushConstantRange colorAttachmentExtentPushConstant{};
        CreatePushConstantRange(colorAttachmentExtentPushConstant, VK_SHADER_STAGE_COMPUTE_BIT,
            sizeof(BlitML::vec2));
        if (!CreatePipelineLayout(m_device, &m_generatePresentationLayout.handle,
            Ce_SinglePointer, &m_generatePresentationImageSetLayout.handle,
            Ce_SinglePointer, &colorAttachmentExtentPushConstant))
        {
            BLIT_ERROR("Failed to create presentation generation pipeline layout");
            return 0;
        }

        // Success
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
            // Persistently mapped pointer
            buffers.viewDataBuffer.pData = 
                reinterpret_cast<BlitzenEngine::CameraViewData*>(
                    buffers.viewDataBuffer.buffer.allocation->GetMappedData());
            // Descriptor write. Only thing that changes per frame is buffer info
            WriteBufferDescriptorSets(buffers.viewDataBuffer.descriptorWrite, 
                buffers.viewDataBuffer.bufferInfo, buffers.viewDataBuffer.descriptorType, 
                buffers.viewDataBuffer.descriptorBinding, 
                buffers.viewDataBuffer.buffer.bufferHandle);

            // Transform buffer is also dynamic
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
            // Persistently mapped pointer (staging buffer)
			buffers.pTransformData = reinterpret_cast<BlitzenEngine::MeshTransform*>(
				buffers.transformStagingBuffer.allocationInfo.pMappedData);
            // Records command to copy staging buffer data to GPU buffers
			BeginCommandBuffer(m_frameToolsList[i].transferCommandBuffer, 
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			CopyBufferToBuffer(m_frameToolsList[i].transferCommandBuffer,
				buffers.transformStagingBuffer.bufferHandle,
				buffers.transformBuffer.buffer.bufferHandle,
				transformBufferSize, 0, 0);
			SubmitCommandBuffer(m_transferQueue.handle, m_frameToolsList[i].transferCommandBuffer);
            vkQueueWaitIdle(m_transferQueue.handle);
        }

        return 1;
    }

    uint8_t VulkanRenderer::StaticBuffersInit(BlitzenEngine::RenderingResources* pResources)
    {
        // The renderer is allow to continue even without render objects but this function should not do anything
        if(pResources->renderObjectCount == 0)
        {
            BLIT_WARN("No objects given to the renderer")
            return 1;// Is this a mistake or intended?
        }

        const auto& vertices = pResources->GetVerticesArray();
        const auto& indices = pResources->GetIndicesArray();
        auto pRenderObjects = pResources->renders;
        auto renderObjectCount = pResources->renderObjectCount;
        const auto& surfaces = pResources->GetSurfaceArray();
        auto pMaterials = pResources->GetMaterialArrayPointer();
        auto materialCount = pResources->GetMaterialCount();
        const auto& clusters = pResources->GetClusterArray();
        const auto& clusterData = pResources->GetClusterIndices();
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
        VkDeviceSize transparentRenderObjectBufferSize = 0;
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

        
        VkDeviceSize clusterBufferSize = 0;
        AllocatedBuffer meshletStagingBuffer;
        VkDeviceSize clusterIndexBufferSize = 0;
        AllocatedBuffer meshletDataStagingBuffer;
        if (BlitzenEngine::Ce_BuildClusters)
        {
            clusterBufferSize = SetupPushDescriptorBuffer(m_device, m_allocator,
                m_currentStaticBuffers.meshletBuffer, meshletStagingBuffer,
                clusters.GetSize(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                clusters.Data());
            if(clusterBufferSize == 0)
            {
                BLIT_ERROR("Failed to create cluster buffer");
                return 0;
            }
            clusterIndexBufferSize = SetupPushDescriptorBuffer(m_device, m_allocator,
                m_currentStaticBuffers.meshletDataBuffer, meshletDataStagingBuffer,
                clusterData.GetSize(),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                clusterData.Data());
            if(clusterIndexBufferSize == 0)
            {
                BLIT_ERROR("Failed to create cluster indices buffer");
                return 0;
            }
        }

        // Mesh shader indirect commands
        VkDeviceSize indirectTaskBufferSize = sizeof(IndirectTaskData) * renderObjectCount;
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
        }

        // Copies the data uploaded to the staging buffer, to the GPU buffers
        CopyStaticBufferDataToGPUBuffers(stagingVertexBuffer.bufferHandle, vertexBufferSize, 
            stagingIndexBuffer.bufferHandle, indexBufferSize, 
            renderObjectStagingBuffer.bufferHandle, renderObjectBufferSize, 
            tranparentRenderObjectStagingBuffer.bufferHandle, transparentRenderObjectBufferSize,
            onpcRenderObjectStagingBuffer.bufferHandle, onpcRenderObjectBufferSize, 
            surfaceStagingBuffer.bufferHandle, surfaceBufferSize, 
            materialStagingBuffer.bufferHandle, materialBufferSize, 
            visibilityBufferSize, meshletStagingBuffer.bufferHandle, clusterBufferSize, 
            meshletDataStagingBuffer.bufferHandle, clusterIndexBufferSize);

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
        VkBuffer transparentObjectStagingBuffer, VkDeviceSize transparentRenderBufferSize,
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

        if (transparentRenderBufferSize != 0)
        {
            CopyBufferToBuffer(commandBuffer, transparentObjectStagingBuffer,
                m_currentStaticBuffers.transparentRenderObjectBuffer.buffer.bufferHandle,
                transparentRenderBufferSize, 0, 0);
        }

        // Render objects that use Oblique Near-Plane Clipping
        if (onpcRenderObjectBufferSize != 0)
        {
            CopyBufferToBuffer(commandBuffer, onpcRenderObjectStagingBuffer,
                m_currentStaticBuffers.onpcReflectiveRenderObjectBuffer.buffer.bufferHandle,
                onpcRenderObjectBufferSize, 0, 0);
        }

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