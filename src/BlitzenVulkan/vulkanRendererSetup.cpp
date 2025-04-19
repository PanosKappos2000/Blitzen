#define VMA_IMPLEMENTATION// Implements vma funcions. Header file included in vulkanData.h
#include "vulkanRenderer.h"
#include "vulkanResourceFunctions.h"
#include "vulkanPipelines.h"
#include "Platform/filesystem.h"

namespace BlitzenVulkan
{
	constexpr size_t ce_textureStagingBufferSize = 128 * 1024 * 1024;

    static uint8_t SetupResourceManagement(VkDevice device, VkPhysicalDevice pdv, VkInstance instance, VmaAllocator& vma,
        VulkanRenderer::FrameTools* frameToolList, VkSampler& textureSampler, Queue graphicsQueue, Queue transferQueue)
    {
        // Initializes VMA allocator which will be used to allocate all Vulkan resources
        // Initialized here since textures can and will be given to Vulkan before the renderer is set up, and they need to be allocated
        if (!CreateVmaAllocator(device, instance, pdv, vma, VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT))
        {
            BLIT_ERROR("Failed to create the vma allocator");
            return 0;
        }

        auto pMemCrucials = MemoryCrucialHandles::GetInstance();
        pMemCrucials->allocator = vma;
        pMemCrucials->device = device;
        pMemCrucials->instance = instance;

        // This sampler will be used by all textures for now
        // Initialized here since textures can and will be given to Vulkan before the renderer is set up
        textureSampler = CreateSampler(device, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_REPEAT);
        if (textureSampler == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create texture sampler");
            return 0;
        }

        // Creates command buffers and synchronization structures in the frame tools struct
        // Created on first Vulkan Initialization stage, because can and will be uploaded to Vulkan early
        for(size_t i = 0; i < ce_framesInFlight; ++i)
        if (!frameToolList[i].Init(device, graphicsQueue, transferQueue))
        {
            BLIT_ERROR("Failed to create frame tools");
            return 0;
        }

        // Success
        return 1;
    }

    static uint8_t LoadDDSImageData(BlitzenEngine::DDS_HEADER& header,
        BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::FileHandle& handle,
        VkFormat& vulkanImageFormat, void* pData)
    {
        vulkanImageFormat = BlitzenVulkan::GetDDSVulkanFormat(header, header10);
        if (vulkanImageFormat == VK_FORMAT_UNDEFINED)
        {
            return 0;
        }

        auto file = reinterpret_cast<FILE*>(handle.pHandle);
        unsigned int blockSize =
            (vulkanImageFormat == VK_FORMAT_BC1_RGBA_UNORM_BLOCK
                || vulkanImageFormat == VK_FORMAT_BC4_SNORM_BLOCK
                || vulkanImageFormat == VK_FORMAT_BC4_UNORM_BLOCK) ?
            8 : 16;
        auto imageSize = BlitzenEngine::GetDDSImageSizeBC(header.dwWidth, header.dwHeight,
            header.dwMipMapCount, blockSize);
        auto readSize = fread(pData, 1, imageSize, file);

        // Checks for image read errors
        if (!pData)
        {
            return 0;
        }
        if (readSize != imageSize)
        {
            return 0;
        }

        // Success
        return 1;
    }

    uint8_t VulkanRenderer::UploadTexture(void* pData, const char* filepath) 
    {
        // Checks if resource management has been setup
        if(!m_stats.bResourceManagementReady)
        {
            // Calls the function and checks if it succeeded. If it did not, it fails
            m_stats.bResourceManagementReady = 
                SetupResourceManagement(m_device, m_physicalDevice, m_instance, m_allocator, 
                    m_frameToolsList, m_textureSampler.handle, m_graphicsQueue, m_transferQueue);
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

    static uint8_t RenderingAttachmentsInit(VkDevice device, VmaAllocator vma,
        PushDescriptorImage& colorAttachment, VkRenderingAttachmentInfo& colorAttachmentInfo,
        PushDescriptorImage& depthAttachment, VkRenderingAttachmentInfo& depthAttachmentInfo,
        PushDescriptorImage& depthPyramid, uint8_t& depthPyramidMipCount, VkImageView* depthPyramidMips,
        VkExtent2D drawExtent, VkExtent2D& depthPyramidExtent)
    {
        /*
            Color attachment handle and value configuration
        */
        // Color attachment sammpler will be used to copy to the swapchain image
        if (!colorAttachment.SamplerInit(device, VK_FILTER_NEAREST,
            VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, nullptr))
        {
            BLIT_ERROR("Failed to create color attachment sampler");
            return 0;
        }
        // Create depth attachment image and image view
        if (!CreatePushDescriptorImage(device, vma, colorAttachment,
            { drawExtent.width, drawExtent.height, 1 }, // extent
            ce_colorAttachmentFormat, ce_colorAttachmentImageUsage,
            1, VMA_MEMORY_USAGE_GPU_ONLY
        ))
        {
            BLIT_ERROR("Failed to create color attachment image resource");
            return 0;
        }
        // Rendering attachment info, most values stay constant
        CreateRenderingAttachmentInfo(colorAttachmentInfo, colorAttachment.image.imageView,
            ce_ColorAttachmentLayout, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
            ce_WindowClearColor);
        /*
            Depth attachement value and handle configuration
        */
        // Creates a sampler for the depth attachment, used for the depth pyramid
        VkSamplerReductionModeCreateInfo reductionInfo{};
        reductionInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reductionInfo.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
        if (!depthAttachment.SamplerInit(device, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST,
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, &reductionInfo))
        {
            BLIT_ERROR("Failed to create depth attachment sampler");
            return 0;
        }
        // Creates rendering attachment image resource for depth attachment
        if (!CreatePushDescriptorImage(device, vma, depthAttachment,
            { drawExtent.width, drawExtent.height, 1 },
            ce_depthAttachmentFormat, ce_depthAttachmentImageUsage,
            1, VMA_MEMORY_USAGE_GPU_ONLY))
        {
            BLIT_ERROR("Failed to create depth attachment image resource");
            return 0;
        }
        // Rendering attachment info info, most values stay constant
        CreateRenderingAttachmentInfo(depthAttachmentInfo, depthAttachment.image.imageView,
            ce_DepthAttachmentLayout, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
            { 0, 0, 0, 0 }, { 0, 0 });
        // Create the depth pyramid image and its mips that will be used for occlusion culling
        if (!CreateDepthPyramid(depthPyramid, depthPyramidExtent,
            depthPyramidMips, depthPyramidMipCount,
            drawExtent, device, vma))
        {
            BLIT_ERROR("Failed to create the depth pyramid");
            return 0;
        }

        // Success
        return 1;
    }

    static VkDescriptorSetLayout CreateGPUBufferPushDescriptorBindings(VkDevice device, VkDescriptorSetLayoutBinding* pBindings,
        uint32_t bindingCount, VulkanRenderer::VarBuffers& varBuffers, VulkanRenderer::StaticBuffers& staticBuffers,
        uint8_t bRaytracing, uint8_t bMeshShaders)
    {
        uint32_t currentId = 0;
        // This logic is terrible and needs to be fixed, TODO
        uint32_t viewDataBindingID = currentId++;
        uint32_t vertexBindingID = currentId++;
        uint32_t depthImageBindingID = currentId++;
        uint32_t renderObjectsBindingID = currentId++;
        uint32_t transformsBindingID = currentId++;
        uint32_t materialsBindingID = currentId++;
        uint32_t indirectCommandsBindingID = currentId++;
        uint32_t indirectCountBindingID = currentId++;
        uint32_t objectVisibilitiesBindingID = currentId++;
        uint32_t primitivesBindingID = currentId++;
        uint32_t clustersBindingID = currentId++;
        uint32_t clusterIndicesBindingID = currentId++;
        uint32_t onpcObjectsBindingID = currentId++;

        uint32_t tlasBindingID;
        if (bRaytracing)
        {
            tlasBindingID = currentId++;
        }
        uint32_t dispatchClusterCommandsBindingID;
        uint32_t clusterCountBindingID;
        if (BlitzenEngine::Ce_BuildClusters)
        {
            dispatchClusterCommandsBindingID = currentId++;
            clusterCountBindingID = currentId++;
        }

        // Every binding in the pushDescriptorSetLayout will have one descriptor
        constexpr uint32_t descriptorCountOfEachPushDescriptorLayoutBinding = 1;

        auto viewDataShaderStageFlags = bMeshShaders ?
            VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT :
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[viewDataBindingID],
            varBuffers.viewDataBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            varBuffers.viewDataBuffer.descriptorType,
            viewDataShaderStageFlags);

        auto vertexBufferShaderStageFlags = bMeshShaders ?
            VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[vertexBindingID],
            staticBuffers.vertexBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.vertexBuffer.descriptorType,
            vertexBufferShaderStageFlags);

        auto surfaceBufferShaderStageFlags = bMeshShaders ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[primitivesBindingID],
            staticBuffers.surfaceBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.surfaceBuffer.descriptorType,
            surfaceBufferShaderStageFlags);

        auto clusterBufferShaderStageFlags = bMeshShaders ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[clustersBindingID],
            staticBuffers.clusterBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.clusterBuffer.descriptorType,
            clusterBufferShaderStageFlags);

        auto clusterDataBufferShaderStageFlags = bMeshShaders ?
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[clusterIndicesBindingID],
            staticBuffers.meshletDataBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.meshletDataBuffer.descriptorType,
            clusterDataBufferShaderStageFlags);

        if (BlitzenEngine::Ce_BuildClusters)
        {
            CreateDescriptorSetLayoutBinding(pBindings[dispatchClusterCommandsBindingID],
                staticBuffers.clusterDispatchBuffer.descriptorBinding,
                descriptorCountOfEachPushDescriptorLayoutBinding,
                staticBuffers.clusterDispatchBuffer.descriptorType,
                VK_SHADER_STAGE_COMPUTE_BIT);

            CreateDescriptorSetLayoutBinding(pBindings[clusterCountBindingID],
                staticBuffers.clusterCountBuffer.descriptorBinding,
                descriptorCountOfEachPushDescriptorLayoutBinding,
                staticBuffers.clusterCountBuffer.descriptorType,
                VK_SHADER_STAGE_COMPUTE_BIT);
        }

        CreateDescriptorSetLayoutBinding(pBindings[depthImageBindingID],
            Ce_DepthPyramidImageBindingID, descriptorCountOfEachPushDescriptorLayoutBinding,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[renderObjectsBindingID],
            staticBuffers.renderObjectBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.renderObjectBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[transformsBindingID],
            varBuffers.transformBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            varBuffers.transformBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[materialsBindingID],
            staticBuffers.materialBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.materialBuffer.descriptorType,
            VK_SHADER_STAGE_FRAGMENT_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[indirectCommandsBindingID],
            staticBuffers.indirectDrawBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.indirectDrawBuffer.descriptorType,
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[indirectCountBindingID],
            staticBuffers.indirectCountBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.indirectCountBuffer.descriptorType,
            VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[objectVisibilitiesBindingID],
            staticBuffers.visibilityBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.visibilityBuffer.descriptorType,
            VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[onpcObjectsBindingID],
            staticBuffers.onpcReflectiveRenderObjectBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.onpcReflectiveRenderObjectBuffer.descriptorType,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        if (bRaytracing)
        {
            CreateDescriptorSetLayoutBinding(pBindings[tlasBindingID],
                staticBuffers.tlasBuffer.descriptorBinding,
                descriptorCountOfEachPushDescriptorLayoutBinding,
                staticBuffers.tlasBuffer.descriptorType,
                VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        return CreateDescriptorSetLayout(device, bindingCount, pBindings,
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
    }

    static uint8_t CreateDescriptorLayouts(VkDevice device, VkDescriptorSetLayout& ssboPushDescriptorLayout,
        VulkanRenderer::VarBuffers& varBuffers, VulkanRenderer::StaticBuffers& staticBuffers,
        uint8_t bRaytracing, uint8_t bMeshShaders, uint32_t textureCount, VkDescriptorSetLayout& textureSetLayout,
        const PushDescriptorImage& depthAttachment, const PushDescriptorImage& depthPyramid,
        VkDescriptorSetLayout& depthPyramidSetLayout, const PushDescriptorImage& colorAttachment,
        VkDescriptorSetLayout& presentationSetLayout)
    {
        constexpr uint32_t descriptorCountOfEachPushDescriptorLayoutBinding = 1;

        // The big GPU push descriptor set layout. Holds most buffers
        BlitCL::DynamicArray<VkDescriptorSetLayoutBinding> gpuPushDescriptorBindings{ 13, {} };
        if (bRaytracing)
        {
            gpuPushDescriptorBindings.PushBack({});
        }
        if (BlitzenEngine::Ce_BuildClusters)
        {
            gpuPushDescriptorBindings.PushBack({});
            gpuPushDescriptorBindings.PushBack({});
        }
        ssboPushDescriptorLayout = CreateGPUBufferPushDescriptorBindings(device,
            gpuPushDescriptorBindings.Data(), (uint32_t)gpuPushDescriptorBindings.GetSize(),
            varBuffers, staticBuffers, bRaytracing, bMeshShaders);
        if (ssboPushDescriptorLayout == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create GPU buffer push descriptor layout");
            return 0;
        }

        // Descriptor set layout for textures
        VkDescriptorSetLayoutBinding texturesLayoutBinding{};
        CreateDescriptorSetLayoutBinding(texturesLayoutBinding, 0, static_cast<uint32_t>(textureCount),
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        textureSetLayout = CreateDescriptorSetLayout(device, 1, &texturesLayoutBinding);
        if (textureSetLayout == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create texture descriptor set layout");
            return 0;
        }

        // Depth pyramid generation layout
        constexpr uint32_t Ce_DepthPyramidGenerationBindingCount = 2;
        BlitCL::StaticArray<VkDescriptorSetLayoutBinding, Ce_DepthPyramidGenerationBindingCount>
            depthPyramidBindings{ {} };
        CreateDescriptorSetLayoutBinding(depthPyramidBindings[0], depthPyramid.m_descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, depthPyramid.m_descriptorType,
            VK_SHADER_STAGE_COMPUTE_BIT);
        CreateDescriptorSetLayoutBinding(depthPyramidBindings[1], depthAttachment.m_descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, depthAttachment.m_descriptorType,
            VK_SHADER_STAGE_COMPUTE_BIT);
        depthPyramidSetLayout = CreateDescriptorSetLayout(device, Ce_DepthPyramidGenerationBindingCount,
            depthPyramidBindings.Data(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if (depthPyramidSetLayout == VK_NULL_HANDLE)
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
            colorAttachment.m_descriptorBinding, descriptorCountOfEachPushDescriptorLayoutBinding,
            colorAttachment.m_descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);
        presentationSetLayout = CreateDescriptorSetLayout(device,
            Ce_PresentationGenerationBindingCount, presentGenerationBindings.Data(),
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if (presentationSetLayout == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create present image generation layout")
                return 0;
        }

        // Success 
        return 1;
    }

    static uint8_t VarBuffersInit(VkDevice device, VmaAllocator vma, VkCommandBuffer commandBuffer, VkQueue queue,
        BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& transforms, VulkanRenderer::VarBuffers* varBuffers)
    {
        for (size_t i = 0; i < ce_framesInFlight; ++i)
        {
            auto& buffers = varBuffers[i];

            // Creates the uniform buffer for view data
            if (!CreateBuffer(vma, buffers.viewDataBuffer.buffer,
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
                SetupPushDescriptorBuffer(device, vma,buffers.transformBuffer,
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
            BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            CopyBufferToBuffer(commandBuffer, buffers.transformStagingBuffer.bufferHandle,
                buffers.transformBuffer.buffer.bufferHandle, transformBufferSize, 0, 0);
            SubmitCommandBuffer(queue, commandBuffer);
            vkQueueWaitIdle(queue);
        }

        return 1;
    }

    static void CopyStaticBufferDataToGPUBuffers(VkCommandBuffer commandBuffer, VkQueue queue,
        VulkanRenderer::StaticBuffers& currentStaticBuffers,
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
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // Vertex data copy
        CopyBufferToBuffer(commandBuffer, stagingVertexBuffer,
            currentStaticBuffers.vertexBuffer.buffer.bufferHandle,
            vertexBufferSize, 0, 0);

        // Index data copy
        CopyBufferToBuffer(commandBuffer, stagingIndexBuffer,
            currentStaticBuffers.indexBuffer.bufferHandle,
            indexBufferSize, 0, 0);

        // Render objects
        CopyBufferToBuffer(commandBuffer, renderObjectStagingBuffer,
            currentStaticBuffers.renderObjectBuffer.buffer.bufferHandle,
            renderObjectBufferSize, 0, 0);

        if (transparentRenderBufferSize != 0)
        {
            CopyBufferToBuffer(commandBuffer, transparentObjectStagingBuffer,
                currentStaticBuffers.transparentRenderObjectBuffer.buffer.bufferHandle,
                transparentRenderBufferSize, 0, 0);
        }

        // Render objects that use Oblique Near-Plane Clipping
        if (onpcRenderObjectBufferSize != 0)
        {
            CopyBufferToBuffer(commandBuffer, onpcRenderObjectStagingBuffer,
                currentStaticBuffers.onpcReflectiveRenderObjectBuffer.buffer.bufferHandle,
                onpcRenderObjectBufferSize, 0, 0);
        }

        // Primitive surfaces
        CopyBufferToBuffer(commandBuffer, surfaceStagingBuffer,
            currentStaticBuffers.surfaceBuffer.buffer.bufferHandle,
            surfaceBufferSize, 0, 0);

        // Materials
        CopyBufferToBuffer(commandBuffer, materialStagingBuffer,
            currentStaticBuffers.materialBuffer.buffer.bufferHandle,
            materialBufferSize, 0, 0);

        // Visibility buffer will start with only zeroes. The first frame will do noting, but that should be fine
        vkCmdFillBuffer(commandBuffer, currentStaticBuffers.visibilityBuffer.buffer.bufferHandle,
            0, visibilityBufferSize, 0
        );

        if (BlitzenEngine::Ce_BuildClusters)
        {
            // Copies the cluster data held by the staging buffer to the meshlet buffer
            CopyBufferToBuffer(commandBuffer, clusterStagingBuffer,
                currentStaticBuffers.clusterBuffer.buffer.bufferHandle,
                clusterBufferSize, 0, 0);

            CopyBufferToBuffer(commandBuffer, clusterIndicesStagingBuffer,
                currentStaticBuffers.meshletDataBuffer.buffer.bufferHandle, clusterIndicesBufferSize,
                0, 0);
        }

        // Submit the commands and wait for the queue to finish
        SubmitCommandBuffer(queue, commandBuffer);
        vkQueueWaitIdle(queue);
    }

    static void SetupGpuBufferDescriptorWriteArrays(const VulkanRenderer::StaticBuffers& m_currentStaticBuffers,
        const VulkanRenderer::VarBuffers& varBuffers,
        VkWriteDescriptorSet* pushDescriptorWritesGraphics, VkWriteDescriptorSet* pushDescriptorWritesCompute)
    {
        if constexpr (BlitzenEngine::Ce_BuildClusters)
        {
            pushDescriptorWritesGraphics[ce_viewDataWriteElement] = varBuffers.viewDataBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[1] = m_currentStaticBuffers.vertexBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[2] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[3] = varBuffers.transformBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[4] = m_currentStaticBuffers.materialBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[5] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[7] = m_currentStaticBuffers.tlasBuffer.descriptorWrite;

            pushDescriptorWritesCompute[ce_viewDataWriteElement] = varBuffers.viewDataBuffer.descriptorWrite;
            pushDescriptorWritesCompute[1] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite;
            pushDescriptorWritesCompute[2] = varBuffers.transformBuffer.descriptorWrite;
            pushDescriptorWritesCompute[3] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
            pushDescriptorWritesCompute[4] = m_currentStaticBuffers.indirectCountBuffer.descriptorWrite;
            pushDescriptorWritesCompute[5] = m_currentStaticBuffers.visibilityBuffer.descriptorWrite;
            pushDescriptorWritesCompute[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;
            pushDescriptorWritesCompute[7] = m_currentStaticBuffers.clusterDispatchBuffer.descriptorWrite;
            pushDescriptorWritesCompute[8] = m_currentStaticBuffers.clusterCountBuffer.descriptorWrite;
            pushDescriptorWritesCompute[9] = {};
        }
        else
        {
            pushDescriptorWritesGraphics[ce_viewDataWriteElement] = varBuffers.viewDataBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[1] = m_currentStaticBuffers.vertexBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[2] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[3] = varBuffers.transformBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[4] = m_currentStaticBuffers.materialBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[5] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[7] = m_currentStaticBuffers.tlasBuffer.descriptorWrite;

            pushDescriptorWritesCompute[ce_viewDataWriteElement] = varBuffers.viewDataBuffer.descriptorWrite;
            pushDescriptorWritesCompute[1] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite;
            pushDescriptorWritesCompute[2] = varBuffers.transformBuffer.descriptorWrite;
            pushDescriptorWritesCompute[3] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
            pushDescriptorWritesCompute[4] = m_currentStaticBuffers.indirectCountBuffer.descriptorWrite;
            pushDescriptorWritesCompute[5] = m_currentStaticBuffers.visibilityBuffer.descriptorWrite;
            pushDescriptorWritesCompute[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;
            pushDescriptorWritesCompute[7] = {};
        }
    }

    static uint8_t CreatePipelineLayouts(VkDevice device, VkDescriptorSetLayout bufferPushDescriptorLayout,
        VkDescriptorSetLayout textureSetLayout, VkPipelineLayout* mainGraphicsLayout, VkPipelineLayout* drawCullLayout,
        VkDescriptorSetLayout depthPyramidSetLayout, VkPipelineLayout* depthPyramidGenerationLayout,
        VkPipelineLayout* onpcGeometryLayout, VkDescriptorSetLayout presentationSetLayout,
        VkPipelineLayout* generatePresentationLayout)
    {

        // Grapchics pipeline layout
        VkDescriptorSetLayout defaultGraphicsPipelinesDescriptorSetLayouts[2] =
        {
             bufferPushDescriptorLayout,
            textureSetLayout
        };
        VkPushConstantRange globalShaderDataPushContant{};
        CreatePushConstantRange(globalShaderDataPushContant, VK_SHADER_STAGE_VERTEX_BIT,
            sizeof(GlobalShaderDataPushConstant));
        if (!CreatePipelineLayout(device, mainGraphicsLayout, BLIT_ARRAY_SIZE(defaultGraphicsPipelinesDescriptorSetLayouts),
            defaultGraphicsPipelinesDescriptorSetLayouts, Ce_SinglePointer, &globalShaderDataPushContant))
        {
            BLIT_ERROR("Failed to create main graphics pipeline layout");
            return 0;
        }

        // Culling shader shared layout (slight differences are not enough to create permutations
        VkPushConstantRange cullShaderPushConstant{};
        CreatePushConstantRange(cullShaderPushConstant, VK_SHADER_STAGE_COMPUTE_BIT,
            sizeof(DrawCullShaderPushConstant));
        if (!CreatePipelineLayout(device, drawCullLayout, Ce_SinglePointer, &bufferPushDescriptorLayout,
            Ce_SinglePointer, &cullShaderPushConstant))
        {
            BLIT_ERROR("Failed to create culling pipeline layout");
            return 0;
        }

        // Layout for depth pyramid generation pipeline
        VkPushConstantRange depthPyramidMipExtentPushConstant{};
        CreatePushConstantRange(depthPyramidMipExtentPushConstant, VK_SHADER_STAGE_COMPUTE_BIT,
            sizeof(BlitML::vec2));
        if (!CreatePipelineLayout(device, depthPyramidGenerationLayout, Ce_SinglePointer, &depthPyramidSetLayout,
            Ce_SinglePointer, &depthPyramidMipExtentPushConstant))
        {
            BLIT_ERROR("Failed to create depth pyramid generation pipeline layout")
                return 0;
        }

        // I need to remove this one
        VkPushConstantRange onpcMatrixPushConstant{};
        CreatePushConstantRange(onpcMatrixPushConstant, VK_SHADER_STAGE_VERTEX_BIT, sizeof(BlitML::mat4));
        if (!CreatePipelineLayout(device, onpcGeometryLayout,
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
        if (!CreatePipelineLayout(device, generatePresentationLayout, Ce_SinglePointer, &presentationSetLayout,
            Ce_SinglePointer, &colorAttachmentExtentPushConstant))
        {
            BLIT_ERROR("Failed to create presentation generation pipeline layout");
            return 0;
        }

        // Success
        return 1;
    }





    uint8_t VulkanRenderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources, 
        float& pyramidWidth, float& pyramidHeight)
    {
        // Checks if resource management has been setup
        if (!m_stats.bResourceManagementReady)
        {
            // Calls the function and checks if it succeeded. If it did not, it fails
            m_stats.bResourceManagementReady =
                SetupResourceManagement(m_device, m_physicalDevice, m_instance, m_allocator,
                    m_frameToolsList, m_textureSampler.handle, m_graphicsQueue, m_transferQueue);
            if (!m_stats.bResourceManagementReady)
            {
                BLIT_ERROR("Failed to setup resource management for Vulkan");
                return 0;
            }
        }

        if (!RenderingAttachmentsInit(m_device, m_allocator, m_colorAttachment, m_colorAttachmentInfo, 
            m_depthAttachment, m_depthAttachmentInfo, m_depthPyramid, m_depthPyramidMipLevels, m_depthPyramidMips, 
            m_drawExtent, m_depthPyramidExtent))
        {
            BLIT_ERROR("Failed to create rendering attachments");
            return 0;
        }

        if(!CreateDescriptorLayouts(m_device, m_pushDescriptorBufferLayout.handle, m_varBuffers[0], m_currentStaticBuffers, 
            m_stats.bRayTracingSupported, m_stats.meshShaderSupport, (uint32_t)textureCount, m_textureDescriptorSetlayout.handle, 
            m_depthAttachment, m_depthPyramid, m_depthPyramidDescriptorLayout.handle, m_colorAttachment, 
            m_generatePresentationImageSetLayout.handle))
        {
            BLIT_ERROR("Failed to create descriptor set layouts");
            return 0;
        }

        if (!CreatePipelineLayouts(m_device, m_pushDescriptorBufferLayout.handle, m_textureDescriptorSetlayout.handle, 
            &m_graphicsPipelineLayout.handle, &m_drawCullLayout.handle, m_depthPyramidDescriptorLayout.handle, 
            &m_depthPyramidGenerationLayout.handle, &m_onpcReflectiveGeometryLayout.handle, 
            m_generatePresentationImageSetLayout.handle, &m_generatePresentationLayout.handle))
        {
            BLIT_ERROR("Failed to create pipeline layouts");
            return 0;
        }

        if(!VarBuffersInit(m_device, m_allocator, m_frameToolsList[0].transferCommandBuffer, 
            m_transferQueue.handle, pResources->transforms, m_varBuffers))
        {
            BLIT_ERROR("Failed to create uniform buffers");
            return 0;
        }

        if(!StaticBuffersInit(pResources))
        {
            BLIT_ERROR("Failed to upload data to the GPU");
            return 0;
        }

        SetupGpuBufferDescriptorWriteArrays(m_currentStaticBuffers, m_varBuffers[0], 
            pushDescriptorWritesGraphics.Data(), pushDescriptorWritesCompute);

        if (!CreateComputeShaders(m_device, &m_initialDrawCullPipeline.handle, &m_lateDrawCullPipeline.handle, 
            &m_onpcDrawCullPipeline.handle, &m_transparentDrawCullPipeline.handle, m_drawCullLayout.handle, 
            &m_depthPyramidGenerationPipeline.handle, m_depthPyramidGenerationLayout.handle, 
            &m_generatePresentationPipeline.handle, m_generatePresentationLayout.handle))
        {
            BLIT_ERROR("Failed to create compute shaders");
            return 0;
        }
        
        // Create the graphics pipeline object 
        if(!SetupMainGraphicsPipeline(m_device, m_stats.meshShaderSupport, &m_opaqueGeometryPipeline.handle, 
            &m_postPassGeometryPipeline.handle, m_graphicsPipelineLayout.handle, 
            &m_onpcReflectiveGeometryPipeline.handle, m_onpcReflectiveGeometryLayout.handle))
        {
            BLIT_ERROR("Failed to create the primary graphics pipeline object");
            return 0;
        }

        // Updates the reference to the depth pyramid width held by the camera
        pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
        pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);

        return 1;
    }

    uint8_t VulkanRenderer::FrameTools::Init(VkDevice device, Queue graphicsQueue, Queue transferQueue)
    {
        // Main command buffer
        VkCommandPoolCreateInfo mainCommandPoolInfo {};
        mainCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        mainCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        mainCommandPoolInfo.queueFamilyIndex = graphicsQueue.index;
        if (vkCreateCommandPool(device, &mainCommandPoolInfo, nullptr, &mainCommandPool.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create command pool for main command buffer");
            return 0;
        }
        VkCommandBufferAllocateInfo mainCommandBufferInfo{};
        mainCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        mainCommandBufferInfo.pNext = nullptr;
        mainCommandBufferInfo.commandBufferCount = 1;
        mainCommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        mainCommandBufferInfo.commandPool = mainCommandPool.handle;
        if (vkAllocateCommandBuffers(device, &mainCommandBufferInfo, &commandBuffer) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create main command buffer");
            return 0;
        }

        // Dedicated transfer command buffer
		VkCommandPoolCreateInfo dedicatedCommandPoolsInfo{};
		dedicatedCommandPoolsInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		dedicatedCommandPoolsInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        dedicatedCommandPoolsInfo.queueFamilyIndex = transferQueue.index;
        if (vkCreateCommandPool(device, &dedicatedCommandPoolsInfo, nullptr, &transferCommandPool.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create dedicated transfer command buffer pool");
            return 0;
        }
        VkCommandBufferAllocateInfo dedicatedCmbInfo{};
		dedicatedCmbInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		dedicatedCmbInfo.pNext = nullptr;
		dedicatedCmbInfo.commandBufferCount = 1;
		dedicatedCmbInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        dedicatedCmbInfo.commandPool = transferCommandPool.handle;
        if (vkAllocateCommandBuffers(device, &dedicatedCmbInfo, &transferCommandBuffer) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create dedicated transfer command buffer")
            return 0;
        }

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceInfo.pNext = nullptr;
        if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create fence");
            return 0;
        }

        VkSemaphoreCreateInfo semaphoresInfo{};
        semaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoresInfo.flags = 0;
        semaphoresInfo.pNext = nullptr;
        if (vkCreateSemaphore(device, &semaphoresInfo, nullptr, &imageAcquiredSemaphore.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for swapchain image acquire");
            return 0;
        }
        if (vkCreateSemaphore(device, &semaphoresInfo, nullptr, &readyToPresentSemaphore.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create semaphore for presentation");
            return 0;
        }
        if (vkCreateSemaphore(device, &semaphoresInfo, nullptr, &buffersReadySemaphore.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create fence for var buffer data copy");
            return 0;
        }

        // Success
        return 1;
    }

    uint8_t CreateDepthPyramid(PushDescriptorImage& depthPyramidImage, VkExtent2D& depthPyramidExtent, 
        VkImageView* depthPyramidMips, uint8_t& depthPyramidMipLevels, 
        VkExtent2D drawExtent, VkDevice device, VmaAllocator allocator)
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

    uint8_t VulkanRenderer::StaticBuffersInit(BlitzenEngine::RenderingResources* pResources)
    {
        // The renderer is allow to continue even without render objects but this function should not do anything
        if(pResources->renderObjectCount == 0)
        {
            BLIT_WARN("No objects given to the renderer")
            return 1;// Is this a mistake or intended?
        }

        constexpr uint32_t SingleElementBuffer = 1;

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
                m_currentStaticBuffers.indirectCountBuffer, SingleElementBuffer,
                VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT 
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
        VkDeviceSize clusterDispatchBufferSize = 0;
        if (BlitzenEngine::Ce_BuildClusters)
        {
            clusterBufferSize = SetupPushDescriptorBuffer(m_device, m_allocator,
                m_currentStaticBuffers.clusterBuffer, meshletStagingBuffer,
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

            clusterDispatchBufferSize =
                SetupPushDescriptorBuffer<ClusterDispatchData>(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY,
                    m_currentStaticBuffers.clusterDispatchBuffer, renderObjectCount,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
            if (clusterDispatchBufferSize == 0)
            {
                BLIT_ERROR("Failed to create indirect dispatch cluster buffer");
                return 0;
            }

            if (!SetupPushDescriptorBuffer<uint32_t>(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY,
                m_currentStaticBuffers.clusterCountBuffer, SingleElementBuffer,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
            {
                BLIT_ERROR("Failed to create indirect count buffer");
                return 0;
            }

            if (!CreateBuffer(m_allocator, m_currentStaticBuffers.clusterCountCopyBuffer,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT))
            {
                BLIT_ERROR("Failed to create indirect count copy buffer");
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
        auto commandBuffer = m_frameToolsList[0].transferCommandBuffer;
        CopyStaticBufferDataToGPUBuffers(commandBuffer, m_transferQueue.handle, 
            m_currentStaticBuffers, stagingVertexBuffer.bufferHandle, vertexBufferSize, 
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
}