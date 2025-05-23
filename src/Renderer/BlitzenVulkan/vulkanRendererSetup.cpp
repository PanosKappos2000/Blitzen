#define VMA_IMPLEMENTATION// Implements vma funcions. Header file included in vulkanData.h
#include "vulkanResourceFunctions.h"
#include "vulkanCommands.h"
#include "vulkanRenderer.h"
#include "vulkanPipelines.h"
#include "Platform/filesystem.h"

namespace BlitzenVulkan
{
    static uint8_t LoadDDSImageData(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10, BlitzenPlatform::C_FILE_SCOPE& scopedFILE,
        VkFormat& vulkanImageFormat, void* pData)
    {
        vulkanImageFormat = GetDDSVulkanFormat(header, header10);
        if (vulkanImageFormat == VK_FORMAT_UNDEFINED)
        {
            BLIT_ERROR("Could not retrieve valid VkFormat for texture");
            return 0;
        }

        uint32_t blockSize = (vulkanImageFormat == VK_FORMAT_BC1_RGBA_UNORM_BLOCK || vulkanImageFormat == VK_FORMAT_BC4_SNORM_BLOCK || vulkanImageFormat == VK_FORMAT_BC4_UNORM_BLOCK) ? 8 : 16;
        auto imageSize = BlitzenEngine::GetDDSImageSizeBC(header.dwWidth, header.dwHeight, header.dwMipMapCount, blockSize);
        if (imageSize == 0)
        {
            BLIT_ERROR("Texture data size result is 0, cannot load texture");
            return 0;
        }

        auto file = scopedFILE.m_pHandle;
        auto readSize = fread(pData, 1, imageSize, file);

        if (!pData)
        {
            BLIT_ERROR("Failed to initialize texture data");
            return 0;
        }

        if (readSize != imageSize)
        {
            BLIT_ERROR("Texture data size is ambiguous");
            return 0;
        }

        // Success
        return 1;
    }

    uint8_t VulkanRenderer::UploadTexture(const char* filepath) 
    {
        if (!m_stats.bResourceManagementReady)
        {
            BLIT_ERROR("Resource management is not initialized, cannot upload texture");
            return 0;
        }

        // Staging buffer
        AllocatedBuffer stagingBuffer;
        if(!CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
            ce_textureStagingBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("Failed to create staging buffer for texture data copy");
            return 0;
        }
        void* pData{ stagingBuffer.allocationInfo.pMappedData };

        // Initializes necessary data for DDS texture
		BlitzenEngine::DDS_HEADER header{};
		BlitzenEngine::DDS_HEADER_DXT10 header10{};
        BlitzenPlatform::C_FILE_SCOPE scopedFILE{};
        VkFormat format = VK_FORMAT_UNDEFINED;
        if(!BlitzenEngine::OpenDDSImageFile(filepath, header, header10, scopedFILE))
        {
            BLIT_ERROR("Failed to open texture file");
            return 0;
        }
		if (!LoadDDSImageData(header, header10, scopedFILE, format, pData))
		{
            BLIT_ERROR("Failed to load texture data");
            return 0;
		}

        // Creates the texture image for Vulkan by copying the data from the staging buffer
        if(!CreateTextureImage(stagingBuffer, m_device, m_allocator, loadedTextures[textureCount].image, 
        {header.dwWidth, header.dwHeight, 1}, format, VK_IMAGE_USAGE_SAMPLED_BIT, 
        m_frameToolsList[0].transferCommandBuffer, m_transferQueue.handle, header.dwMipMapCount))
        {
            BLIT_ERROR("Failed to load Vulkan texture image");
            return 0;
        }
        
        // Add the global sampler at the element in the array that was just porcessed
        loadedTextures[textureCount].sampler = m_textureSampler.handle;
        textureCount++;
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
        uint32_t lodBufferBindingId = currentId++;
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

        // Every binding in the pushDescriptorSetLayout will have one descriptor
        constexpr uint32_t descriptorCountOfEachPushDescriptorLayoutBinding = 1;

        auto viewDataShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT : 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[viewDataBindingID], varBuffers.viewDataBuffer.descriptorBinding, descriptorCountOfEachPushDescriptorLayoutBinding,
            varBuffers.viewDataBuffer.descriptorType, viewDataShaderStageFlags);

        auto vertexBufferShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT : VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[vertexBindingID], staticBuffers.vertexBuffer.descriptorBinding, descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.vertexBuffer.descriptorType, vertexBufferShaderStageFlags);

        auto surfaceBufferShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[primitivesBindingID], staticBuffers.surfaceBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, staticBuffers.surfaceBuffer.descriptorType, surfaceBufferShaderStageFlags);

        auto clusterBufferShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[clustersBindingID], staticBuffers.clusterBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, staticBuffers.clusterBuffer.descriptorType, clusterBufferShaderStageFlags);

        auto clusterDataBufferShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(pBindings[clusterIndicesBindingID], staticBuffers.meshletDataBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, staticBuffers.meshletDataBuffer.descriptorType, clusterDataBufferShaderStageFlags);

        CreateDescriptorSetLayoutBinding(pBindings[depthImageBindingID], Ce_DepthPyramidImageBindingID, descriptorCountOfEachPushDescriptorLayoutBinding,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[lodBufferBindingId], staticBuffers.lodBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, staticBuffers.lodBuffer.descriptorType, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[transformsBindingID], varBuffers.transformBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, varBuffers.transformBuffer.descriptorType, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[materialsBindingID], staticBuffers.materialBuffer.descriptorBinding, descriptorCountOfEachPushDescriptorLayoutBinding,
            staticBuffers.materialBuffer.descriptorType, VK_SHADER_STAGE_FRAGMENT_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[indirectCommandsBindingID], staticBuffers.indirectDrawBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, staticBuffers.indirectDrawBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[indirectCountBindingID], staticBuffers.indirectCountBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, staticBuffers.indirectCountBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[objectVisibilitiesBindingID], staticBuffers.visibilityBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, staticBuffers.visibilityBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(pBindings[onpcObjectsBindingID], staticBuffers.onpcReflectiveRenderObjectBuffer.descriptorBinding,
            descriptorCountOfEachPushDescriptorLayoutBinding, staticBuffers.onpcReflectiveRenderObjectBuffer.descriptorType, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        if (bRaytracing)
        {
            CreateDescriptorSetLayoutBinding(pBindings[tlasBindingID], staticBuffers.tlasBuffer.descriptorBinding, descriptorCountOfEachPushDescriptorLayoutBinding,
                staticBuffers.tlasBuffer.descriptorType, VK_SHADER_STAGE_FRAGMENT_BIT);
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
        ssboPushDescriptorLayout = CreateGPUBufferPushDescriptorBindings(device, gpuPushDescriptorBindings.Data(), (uint32_t)gpuPushDescriptorBindings.GetSize(),
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
            BLIT_ERROR("Failed to create present image generation layout");
            return 0;
        }

        // Success 
        return 1;
    }

    static uint8_t VarBuffersInit(VkDevice device, VmaAllocator vma, VkCommandBuffer commandBuffer, VkQueue queue,
        BlitzenEngine::DrawContext& context, VulkanRenderer::VarBuffers* varBuffers)
    {
        const auto& transforms{ context.m_renders.m_transforms };
        size_t transformDynamicDataSize{ context.m_renders.m_dynamicTransformCount * sizeof(BlitzenEngine::MeshTransform)};

        for (size_t i = 0; i < ce_framesInFlight; ++i)
        {
            auto& buffers = varBuffers[i];

            // Creates the uniform buffer for view data
            if (!CreateBuffer(vma, buffers.viewDataBuffer.buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                sizeof(BlitzenEngine::CameraViewData), VMA_ALLOCATION_CREATE_MAPPED_BIT))
            {
                BLIT_ERROR("Failed to create view data buffer");
                return 0;
            }
            // Persistently mapped pointer
            buffers.viewDataBuffer.pData = reinterpret_cast<BlitzenEngine::CameraViewData*>(buffers.viewDataBuffer.buffer.allocation->GetMappedData());
            // Descriptor write. Only thing that changes per frame is buffer info
            CreatePushDescriptorWrite(buffers.viewDataBuffer.descriptorWrite, buffers.viewDataBuffer.bufferInfo, 
                buffers.viewDataBuffer.buffer.bufferHandle, buffers.viewDataBuffer.descriptorType, buffers.viewDataBuffer.descriptorBinding);

            // Transform buffer is also dynamic
            AllocatedBuffer transformStagingBufferTemp;
            auto transformBufferSize
            {
                SetupPushDescriptorBuffer(device, vma,buffers.transformBuffer, transformStagingBufferTemp, context.m_renders.m_transformCount,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, context.m_renders.m_transforms)
            };
            if (transformBufferSize == 0)
            {
                BLIT_ERROR("Failed to create transform buffer");
                return 0;
            }
            // Records command to copy staging buffer data to GPU buffers
            BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            CopyBufferToBuffer(commandBuffer, transformStagingBufferTemp.bufferHandle, buffers.transformBuffer.buffer.bufferHandle, transformBufferSize, 0, 0);
            SubmitCommandBuffer(queue, commandBuffer);
            vkQueueWaitIdle(queue);

            // Persistently mapped staging buffer
            CreateBuffer(vma, buffers.transformStagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
                transformDynamicDataSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
            buffers.pTransformData = reinterpret_cast<BlitzenEngine::MeshTransform*>(buffers.transformStagingBuffer.allocationInfo.pMappedData);
            buffers.dynamicTransformDataSize = transformDynamicDataSize;
        }

        return 1;
    }

    struct StaticBufferCopyContext
    {
        VkBuffer stagings[Ce_StaticSSBODataCount]{};
        VkDeviceSize sizes[Ce_StaticSSBODataCount]{};
    };
    static void CopyStaticBufferDataToGPUBuffers(VkCommandBuffer commandBuffer, VkQueue queue, VulkanRenderer::StaticBuffers& buffers, StaticBufferCopyContext& ctx,
        VkDeviceSize visibilityBufferSize)
    {
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_VertexBufferDataCopyIndex], buffers.vertexBuffer.buffer.bufferHandle, 
            ctx.sizes[Ce_VertexBufferDataCopyIndex], 0, 0);

        CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_IndexBufferDataCopyIndex], buffers.indexBuffer.bufferHandle, 
            ctx.sizes[Ce_IndexBufferDataCopyIndex], 0, 0);

        CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_OpaqueRenderBufferCopyIndex], buffers.renderObjectBuffer.bufferHandle, 
            ctx.sizes[Ce_OpaqueRenderBufferCopyIndex], 0, 0);

        if (ctx.sizes[Ce_TransparentRenderBufferCopyIndex] != 0)
        {
            CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_TransparentRenderBufferCopyIndex], buffers.transparentRenderObjectBuffer.bufferHandle, 
                ctx.sizes[Ce_TransparentRenderBufferCopyIndex], 0, 0);
        }

        if (ctx.sizes[Ce_ONPCRenderBufferCopyIndex] != 0)
        {
            CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_ONPCRenderBufferCopyIndex], buffers.onpcReflectiveRenderObjectBuffer.buffer.bufferHandle, 
                ctx.sizes[Ce_ONPCRenderBufferCopyIndex], 0, 0);
        }

        CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_SurfaceBufferDataCopyIndex], buffers.surfaceBuffer.buffer.bufferHandle, 
            ctx.sizes[Ce_SurfaceBufferDataCopyIndex], 0, 0);

		CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_LodBufferDataCopyIndex], buffers.lodBuffer.buffer.bufferHandle, 
            ctx.sizes[Ce_LodBufferDataCopyIndex], 0, 0);

        CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_MaterialBufferDataCopyIndex], buffers.materialBuffer.buffer.bufferHandle, 
            ctx.sizes[Ce_MaterialBufferDataCopyIndex], 0, 0);
        
        // Visibility buffer just gets filled with zeroes
        vkCmdFillBuffer(commandBuffer, buffers.visibilityBuffer.buffer.bufferHandle, 0, visibilityBufferSize, 0);

        if (BlitzenCore::Ce_BuildClusters)
        {
            CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_ClusterBufferDataCopyIndex], buffers.clusterBuffer.buffer.bufferHandle, 
                ctx.sizes[Ce_ClusterBufferDataCopyIndex], 0, 0);

            CopyBufferToBuffer(commandBuffer, ctx.stagings[Ce_ClusterIndexBufferDataCopyIndex], buffers.meshletDataBuffer.buffer.bufferHandle, 
                ctx.sizes[Ce_ClusterIndexBufferDataCopyIndex], 0, 0);
        }

        // Submit the commands and wait for the queue to finish
        SubmitCommandBuffer(queue, commandBuffer);
        vkQueueWaitIdle(queue);
    }

    static uint8_t StaticBuffersInit(VkInstance instance, VkDevice device, VmaAllocator vma, VulkanRenderer::FrameTools& frameTools, VkQueue transferQueue,
        VulkanRenderer::StaticBuffers& staticBuffers, BlitzenEngine::DrawContext& context, VulkanStats& stats)
    {
        const auto& vertices{ context.m_meshes.m_vertices };
        const auto& indices{ context.m_meshes.m_indices };
        auto pRenderObjects { context.m_renders.m_renders};
        auto renderObjectCount{ context.m_renders.m_renderCount };
        const auto& surfaces{ context.m_meshes.m_surfaces };
        auto pMaterials = context.m_textures.m_materials;
        auto materialCount = context.m_textures.m_materialCount;
        const auto& clusters = context.m_meshes.m_clusters;
        const auto& clusterData = context.m_meshes.m_clusterIndices;
        auto pOnpcRenderObjects = context.m_renders.m_onpcRenders;
        auto onpcRenderObjectCount = context.m_renders.m_onpcRenderCount;
        auto transforms = context.m_renders.m_transforms;
        auto transparentRenderobjects = context.m_renders.m_transparentRenders;
        uint32_t transparentRenderCount = context.m_renders.m_transparentRenderCount;
        const auto& lodData = context.m_meshes.m_LODs;

        // Additional RT flags for geometry
        auto bRT{ stats.bRayTracingSupported };
        uint32_t geometryRtFlags = bRT ? VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0;

        // Vertex buffer
        AllocatedBuffer stagingVertexBuffer{};
        auto vertexBufferSize
        {
            SetupPushDescriptorBuffer(device, vma, staticBuffers.vertexBuffer, stagingVertexBuffer, vertices.GetSize(),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | geometryRtFlags, vertices.Data())
        };
        if (vertexBufferSize == 0)
        {
            BLIT_ERROR("Failed to create vertex buffer");
            return 0;
        }

        // Index buffer
        AllocatedBuffer stagingIndexBuffer;
        VkDeviceSize indexBufferSize{ indices.GetSize() * sizeof(uint32_t) };
        if (!CreateSSBO(vma, device, indices.Data(), staticBuffers.indexBuffer, stagingIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, indexBufferSize))
        {
            BLIT_ERROR("Failed to create index buffer");
            return 0;
        }

        // Opaque render buffer
        AllocatedBuffer renderObjectStagingBuffer;
        VkDeviceSize renderObjectBufferSize{ renderObjectCount * sizeof(BlitzenEngine::RenderObject) };
        if (!CreateSSBO(vma, device, pRenderObjects, staticBuffers.renderObjectBuffer, renderObjectStagingBuffer,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, renderObjectBufferSize))
        {
            BLIT_ERROR("Failed to create render object buffer");
            return 0;
        }
        // Address for push constant
        staticBuffers.renderObjectBufferAddress = GetBufferAddress(device, staticBuffers.renderObjectBuffer.bufferHandle);

        // ONPC render buffer
        AllocatedBuffer onpcRenderObjectStagingBuffer;
        VkDeviceSize onpcRenderObjectBufferSize{ 0 };
        if (onpcRenderObjectCount != 0)
        {

            onpcRenderObjectBufferSize = SetupPushDescriptorBuffer(device, vma, staticBuffers.onpcReflectiveRenderObjectBuffer,
                onpcRenderObjectStagingBuffer, onpcRenderObjectCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, pOnpcRenderObjects);
            if (onpcRenderObjectBufferSize == 0)
            {
                BLIT_ERROR("Failed to create Oblique Near-Plane Clipping render object buffer");
                return 0;
            }
            stats.bObliqueNearPlaneClippingObjectsExist = 1;
        }

        // Transparent render buffer
        AllocatedBuffer tranparentRenderObjectStagingBuffer;
        auto transparentRenderObjectBufferSize{ transparentRenderCount * sizeof(BlitzenEngine::RenderObject) };
        if (transparentRenderObjectBufferSize != 0)
        {
            if (!CreateSSBO(vma, device, transparentRenderobjects, staticBuffers.transparentRenderObjectBuffer, tranparentRenderObjectStagingBuffer,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, transparentRenderObjectBufferSize))
            {
                BLIT_ERROR("Failed to create transparent render object buffer");
                return 0;
            }
            stats.bTranspartentObjectsExist = 1;
            staticBuffers.transparentRenderObjectBufferAddress = GetBufferAddress(device, staticBuffers.transparentRenderObjectBuffer.bufferHandle);
        }

        // Surface buffer
        AllocatedBuffer surfaceStagingBuffer;
        auto surfaceBufferSize
        {
            SetupPushDescriptorBuffer(device, vma, staticBuffers.surfaceBuffer, surfaceStagingBuffer, surfaces.GetSize(),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, surfaces.Data())
        };
        if (surfaceBufferSize == 0)
        {
            BLIT_ERROR("Failed to create surface buffer");
            return 0;
        }

        // Lod buffer
        AllocatedBuffer lodStagingBuffer;
        auto lodBufferSize
        {
            SetupPushDescriptorBuffer(device, vma, staticBuffers.lodBuffer, lodStagingBuffer, lodData.GetSize(),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, lodData.Data())
        };
        if (lodBufferSize == 0)
        {
            BLIT_ERROR("Failed to create surface buffer");
            return 0;
        }

        // Mat buffer
        AllocatedBuffer materialStagingBuffer;
        auto materialBufferSize
        {
            SetupPushDescriptorBuffer(device, vma, staticBuffers.materialBuffer, materialStagingBuffer, materialCount,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, pMaterials)
        };
        if (materialBufferSize == 0)
        {
            BLIT_ERROR("Failed to create material buffer");
            return 0;
        }

        // Indirect draw cmd
        auto indirectDrawBufferSize
        {
            SetupPushDescriptorBuffer<IndirectDrawData>(vma, VMA_MEMORY_USAGE_GPU_ONLY, staticBuffers.indirectDrawBuffer, IndirectDrawElementCount,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
        };
        if (indirectDrawBufferSize == 0)
        {
            BLIT_ERROR("Failed to create indirect draw buffer");
            return 0;
        }

        // Indirect draw count
        if (!SetupPushDescriptorBuffer<uint32_t>(vma, VMA_MEMORY_USAGE_GPU_ONLY, staticBuffers.indirectCountBuffer, 1,
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
        {
            BLIT_ERROR("Failed to create indirect count buffer");
            return 0;
        }

        // Visibility buffer, for occlusion culling mechanics
        auto visibilityBufferSize
        {
            SetupPushDescriptorBuffer<uint32_t>(vma, VMA_MEMORY_USAGE_GPU_ONLY, staticBuffers.visibilityBuffer, renderObjectCount,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        };
        if (visibilityBufferSize == 0)
        {
            BLIT_ERROR("Failed to create render object visibility buffer");
            return 0;
        }

        // Cluster mode buffers
        VkDeviceSize clusterBufferSize = 0;
        AllocatedBuffer clusterStagingBuffer;
        VkDeviceSize clusterIndexBufferSize = 0;
        AllocatedBuffer clusterIndexStagingBuffer;
        VkDeviceSize clusterDispatchBufferSize = 0;
        VkDeviceSize transparentClusterDispatchBufferSize = 0;
        if (BlitzenCore::Ce_BuildClusters)
        {
            clusterBufferSize = SetupPushDescriptorBuffer(device, vma, staticBuffers.clusterBuffer, clusterStagingBuffer,
                clusters.GetSize(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, clusters.Data());
            if (clusterBufferSize == 0)
            {
                BLIT_ERROR("Failed to create cluster buffer");
                return 0;
            }

            clusterIndexBufferSize = SetupPushDescriptorBuffer(device, vma, staticBuffers.meshletDataBuffer, clusterIndexStagingBuffer,
                clusterData.GetSize(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, clusterData.Data());
            if (clusterIndexBufferSize == 0)
            {
                BLIT_ERROR("Failed to create cluster indices buffer");
                return 0;
            }

            // Cluster dispatch buffer (cluster data for visible objects)
            clusterDispatchBufferSize = IndirectDrawElementCount * sizeof(ClusterDispatchData);
            if (!CreateBuffer(vma, staticBuffers.clusterDispatchBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, clusterDispatchBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
            {
                BLIT_ERROR("Failed to create indirect dispatch cluster buffer");
                return 0;
            }
            // Device address
            staticBuffers.clusterDispatchBufferAddress = GetBufferAddress(device, staticBuffers.clusterDispatchBuffer.bufferHandle);

            // Cluster count for all visible frame objects
            if (!CreateBuffer(vma, staticBuffers.clusterCountBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY, sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT))
            {
                BLIT_ERROR("Failed to create indirect count buffer");
                return 0;
            }
            // Device address
            staticBuffers.clusterCountBufferAddress = GetBufferAddress(device, staticBuffers.clusterCountBuffer.bufferHandle);

            // CPU side cluster count
            if (!CreateBuffer(vma, staticBuffers.clusterCountCopyBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT))
            {
                BLIT_ERROR("Failed to create indirect count copy buffer");
                return 0;
            }

            // Transparent version of cluster dispatch
            transparentClusterDispatchBufferSize = Ce_TrasparentDispatchElementCount * sizeof(ClusterDispatchData);
            if (!CreateBuffer(vma, staticBuffers.transparentClusterDispatchBuffer,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, transparentClusterDispatchBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
            {
                BLIT_ERROR("Failed to create transparent indirect dispatch cluster buffer");
                return 0;
            }
            // Device address
            staticBuffers.transparentClusterDispatchBufferAddress = GetBufferAddress(device, staticBuffers.transparentClusterDispatchBuffer.bufferHandle);

            // Transparent version of cluster count
            if (!CreateBuffer(vma, staticBuffers.transparentClusterCountBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
                sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT))
            {
                BLIT_ERROR("Failed to create transparent indirect count buffer");
                return 0;
            }
            // Device address
            staticBuffers.transparentClusterCountBufferAddress = GetBufferAddress(device, staticBuffers.transparentClusterCountBuffer.bufferHandle);

            // Transparent Cluster count copy
            if (!CreateBuffer(vma, staticBuffers.transparentClusterCountCopyBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT))
            {
                BLIT_ERROR("Failed to create indirect count copy buffer");
                return 0;
            }
        }

        // Mesh shader cmd
        VkDeviceSize indirectTaskBufferSize = sizeof(IndirectTaskData) * renderObjectCount;
        if (stats.meshShaderSupport)
        {
            if (indirectTaskBufferSize == 0)
            {
                return 0;
            }
            if (!SetupPushDescriptorBuffer<IndirectTaskData>(vma, VMA_MEMORY_USAGE_GPU_ONLY, staticBuffers.indirectTaskBuffer, indirectTaskBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
            {
                BLIT_ERROR("Falied to create indirect task buffer");
                return 0;
            }
        }

        // Data copy to SSBOs
        auto commandBuffer = frameTools.transferCommandBuffer;
        StaticBufferCopyContext copyContext;
        copyContext.stagings[Ce_VertexBufferDataCopyIndex] = stagingVertexBuffer.bufferHandle;
        copyContext.sizes[Ce_VertexBufferDataCopyIndex] = vertexBufferSize;
        copyContext.stagings[Ce_IndexBufferDataCopyIndex] = stagingIndexBuffer.bufferHandle;
        copyContext.sizes[Ce_IndexBufferDataCopyIndex] = indexBufferSize;
        copyContext.stagings[Ce_OpaqueRenderBufferCopyIndex] = renderObjectStagingBuffer.bufferHandle;
        copyContext.sizes[Ce_OpaqueRenderBufferCopyIndex] = renderObjectBufferSize;
        copyContext.stagings[Ce_TransparentRenderBufferCopyIndex] = tranparentRenderObjectStagingBuffer.bufferHandle;
        copyContext.sizes[Ce_TransparentRenderBufferCopyIndex] = transparentRenderObjectBufferSize;
        copyContext.stagings[Ce_SurfaceBufferDataCopyIndex] = surfaceStagingBuffer.bufferHandle;
        copyContext.sizes[Ce_SurfaceBufferDataCopyIndex] = surfaceBufferSize;
        copyContext.stagings[Ce_LodBufferDataCopyIndex] = lodStagingBuffer.bufferHandle;
        copyContext.sizes[Ce_LodBufferDataCopyIndex] = lodBufferSize;
        copyContext.stagings[Ce_MaterialBufferDataCopyIndex] = materialStagingBuffer.bufferHandle;
        copyContext.sizes[Ce_MaterialBufferDataCopyIndex] = materialBufferSize;
        copyContext.stagings[Ce_ClusterBufferDataCopyIndex] = clusterStagingBuffer.bufferHandle;
        copyContext.sizes[Ce_ClusterBufferDataCopyIndex] = clusterBufferSize;
        copyContext.stagings[Ce_ClusterIndexBufferDataCopyIndex] = clusterIndexStagingBuffer.bufferHandle;
        copyContext.sizes[Ce_ClusterIndexBufferDataCopyIndex] = clusterIndexBufferSize;
        CopyStaticBufferDataToGPUBuffers(commandBuffer, transferQueue, staticBuffers, copyContext, visibilityBufferSize);

        // Raytracing
        if (stats.bRayTracingSupported)
        {
            if (!BuildBlas(instance, device, vma, frameTools, transferQueue, context, staticBuffers))
            {
                BLIT_ERROR("Failed to build blas for RT");
                return 0;
            }
            if (!BuildTlas(instance, device, vma, frameTools, transferQueue, staticBuffers, context))
            {
                BLIT_ERROR("Failed to build tlas for RT");
                return 0;
            }
        }

        // SUCCESS
        return 1;
    }

    static void SetupGpuBufferDescriptorWriteArrays(const VulkanRenderer::StaticBuffers& m_currentStaticBuffers, const VulkanRenderer::VarBuffers& varBuffers,
        VkWriteDescriptorSet* pushDescriptorWritesGraphics, VkWriteDescriptorSet* pushDescriptorWritesCompute)
    {
        if constexpr (BlitzenCore::Ce_BuildClusters)
        {
            pushDescriptorWritesGraphics[ce_viewDataWriteElement] = varBuffers.viewDataBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_VertexBufferPushDescriptorId] = m_currentStaticBuffers.vertexBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_MaterialBufferPushDescriptorId] = m_currentStaticBuffers.materialBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_TransformBufferGraphicsDescriptorId] = varBuffers.transformBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_DrawCmdBufferGraphicsDescriptorId] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_SurfaceBufferGraphicsDescriptorId] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;

            pushDescriptorWritesCompute[ce_viewDataWriteElement] = varBuffers.viewDataBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_LodBufferDescriptorId] = m_currentStaticBuffers.lodBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_TransformBufferDrawCullDescriptorId] = varBuffers.transformBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_DrawCmdBufferDrawCullDescriptorId] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_DrawCountBufferDrawCullDescriptorId] = m_currentStaticBuffers.indirectCountBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_VisibilityBufferDrawCullDescriptorId] = m_currentStaticBuffers.visibilityBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_SurfaceBufferDrawCullDescriptorId] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;
			pushDescriptorWritesCompute[Ce_ClusterBufferDrawCullDescriptorId] = m_currentStaticBuffers.clusterBuffer.descriptorWrite;
        }
        else
        {
            pushDescriptorWritesGraphics[ce_viewDataWriteElement] = varBuffers.viewDataBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_VertexBufferPushDescriptorId] = m_currentStaticBuffers.vertexBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_MaterialBufferPushDescriptorId] = m_currentStaticBuffers.materialBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_TransformBufferGraphicsDescriptorId] = varBuffers.transformBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_DrawCmdBufferGraphicsDescriptorId] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
            pushDescriptorWritesGraphics[Ce_SurfaceBufferGraphicsDescriptorId] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;

            pushDescriptorWritesCompute[ce_viewDataWriteElement] = varBuffers.viewDataBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_LodBufferDescriptorId] = m_currentStaticBuffers.lodBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_TransformBufferDrawCullDescriptorId] = varBuffers.transformBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_DrawCmdBufferDrawCullDescriptorId] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_DrawCountBufferDrawCullDescriptorId] = m_currentStaticBuffers.indirectCountBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_VisibilityBufferDrawCullDescriptorId] = m_currentStaticBuffers.visibilityBuffer.descriptorWrite;
            pushDescriptorWritesCompute[Ce_SurfaceBufferDrawCullDescriptorId] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;
        }
    }

    static uint8_t CreatePipelineLayouts(VkDevice device, VkDescriptorSetLayout bufferPushDescriptorLayout,
        VkDescriptorSetLayout textureSetLayout, VkPipelineLayout* mainGraphicsLayout, VkPipelineLayout* drawCullLayout,
        VkDescriptorSetLayout depthPyramidSetLayout, VkPipelineLayout* depthPyramidGenerationLayout,
        VkPipelineLayout* onpcGeometryLayout, VkDescriptorSetLayout presentationSetLayout,
        VkPipelineLayout* generatePresentationLayout, VkPipelineLayout* clusterCullLayout)
    {

        // Grapchics pipeline layout
        VkDescriptorSetLayout defaultGraphicsPipelinesDescriptorSetLayouts[2] =
        {
            bufferPushDescriptorLayout,
            textureSetLayout
        };
        VkPushConstantRange globalShaderDataPushContant{};
        CreatePushConstantRange(globalShaderDataPushContant, VK_SHADER_STAGE_VERTEX_BIT, sizeof(GlobalShaderDataPushConstant));
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

        VkPushConstantRange clusterCullPushConstant{};
        CreatePushConstantRange(clusterCullPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(ClusterCullShaderPushConstant));
        if (!CreatePipelineLayout(device, clusterCullLayout, Ce_SinglePointer, &bufferPushDescriptorLayout,
            Ce_SinglePointer, &clusterCullPushConstant))
        {
            BLIT_ERROR("Failed to create culling pipeline layout");
            return 0;
        }

        // Layout for depth pyramid generation pipeline
        VkPushConstantRange depthPyramidMipExtentPushConstant{};
        CreatePushConstantRange(depthPyramidMipExtentPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(BlitML::vec2));
        if (!CreatePipelineLayout(device, depthPyramidGenerationLayout, Ce_SinglePointer, &depthPyramidSetLayout,
            Ce_SinglePointer, &depthPyramidMipExtentPushConstant))
        {
            BLIT_ERROR("Failed to create depth pyramid generation pipeline layout")
                return 0;
        }

        // I need to remove this one
        VkPushConstantRange onpcMatrixPushConstant{};
        CreatePushConstantRange(onpcMatrixPushConstant, VK_SHADER_STAGE_VERTEX_BIT, sizeof(BlitML::mat4));
        if (!CreatePipelineLayout(device, onpcGeometryLayout, BLIT_ARRAY_SIZE(defaultGraphicsPipelinesDescriptorSetLayouts), defaultGraphicsPipelinesDescriptorSetLayouts,
            Ce_SinglePointer, &onpcMatrixPushConstant))
        {
            BLIT_ERROR("Failed to create onpc graphics pipeline");
            return 0;
        }

        // Generate present image compute shader layout
        VkPushConstantRange colorAttachmentExtentPushConstant{};
        CreatePushConstantRange(colorAttachmentExtentPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(BlitML::vec2));
        if (!CreatePipelineLayout(device, generatePresentationLayout, Ce_SinglePointer, &presentationSetLayout, Ce_SinglePointer, &colorAttachmentExtentPushConstant))
        {
            BLIT_ERROR("Failed to create presentation generation pipeline layout");
            return 0;
        }

        // Success
        return 1;
    }

    static uint8_t AllocateTextureDescriptorSet(VkDevice device, uint32_t textureCount, TextureData* pTextures,
        VkDescriptorPool& descriptorPool, VkDescriptorSetLayout* pLayout, VkDescriptorSet& descriptorSet)
    {
        if (textureCount == 0)
        {
            BLIT_ERROR("There are not textures loaded")
            return 0;
        }
        
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = textureCount;

        descriptorPool = CreateDescriptorPool(device, 1, &poolSize, 1);
        if (descriptorPool == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create descriptor pool for textures");
            return 0;
        }
        
        if (!AllocateDescriptorSets(device, descriptorPool, pLayout, 1, &descriptorSet))
        {
            BLIT_ERROR("Failed to allocate descriptor set for textures");
            return 0;
        }

        // Array of descriptor infos
        BlitCL::DynamicArray<VkDescriptorImageInfo> imageInfos(textureCount);
        for (size_t i = 0; i < imageInfos.GetSize(); ++i)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = pTextures[i].image.imageView;
            imageInfos[i].sampler = pTextures[i].sampler;
        }

        VkWriteDescriptorSet write{};
        WriteImageDescriptorSets(write, imageInfos.Data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorSet, uint32_t(imageInfos.GetSize()), 0);
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

        return 1;
    }


    uint8_t VulkanRenderer::SetupForRendering(BlitzenEngine::DrawContext& context)
    {
        BLIT_ASSERT(m_stats.bResourceManagementReady);

        if (!RenderingAttachmentsInit(m_device, m_allocator, m_colorAttachment, m_colorAttachmentInfo, 
            m_depthAttachment, m_depthAttachmentInfo, m_depthPyramid, m_depthPyramidMipLevels, m_depthPyramidMips, 
            m_drawExtent, m_depthPyramidExtent))
        {
            BLIT_ERROR("Failed to create rendering attachments");
            return 0;
        }

        if(!CreateDescriptorLayouts(m_device, m_pushDescriptorBufferLayout.handle, m_varBuffers[0], m_staticBuffers, 
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
            m_generatePresentationImageSetLayout.handle, &m_generatePresentationLayout.handle, &m_clusterCullLayout.handle))
        {
            BLIT_ERROR("Failed to create pipeline layouts");
            return 0;
        }

        if(!VarBuffersInit(m_device, m_allocator, m_frameToolsList[0].transferCommandBuffer, m_transferQueue.handle, context, m_varBuffers))
        {
            BLIT_ERROR("Failed to create uniform buffers");
            return 0;
        }

        if(!StaticBuffersInit(m_instance, m_device, m_allocator, m_frameToolsList[0], m_transferQueue.handle, m_staticBuffers, context, m_stats))
        {
            BLIT_ERROR("Failed to upload data to the GPU");
            return 0;
        }

        if (!AllocateTextureDescriptorSet(m_device, (uint32_t)textureCount, loadedTextures, m_textureDescriptorPool.handle, &m_textureDescriptorSetlayout.handle,
            m_textureDescriptorSet))
        {
            BLIT_ERROR("Failed to allocate texture descriptor sets");
            return 0;
        }

        SetupGpuBufferDescriptorWriteArrays(m_staticBuffers, m_varBuffers[0], m_graphicsDescriptors, m_drawCullDescriptors);

        if (!CreateComputeShaders(m_device, &m_initialDrawCullPipeline.handle, &m_lateDrawCullPipeline.handle, 
            &m_onpcDrawCullPipeline.handle, &m_transparentDrawCullPipeline.handle, m_drawCullLayout.handle, 
            &m_depthPyramidGenerationPipeline.handle, m_depthPyramidGenerationLayout.handle, 
            &m_generatePresentationPipeline.handle, m_generatePresentationLayout.handle))
        {
            BLIT_ERROR("Failed to create compute shaders");
            return 0;
        }

        if (BlitzenCore::Ce_BuildClusters && !CreateClusterComputePipelines(m_device, &m_preClusterCullPipeline.handle, 
            &m_intialClusterCullPipeline.handle, &m_transparentClusterCullPipeline.handle, m_clusterCullLayout.handle))
        {
            BLIT_ERROR("Failed to create cluster shaders");
            return 0;
        }
        
        // Create the graphics pipeline object 
        if(!CreateGraphicsPipelines(m_device, m_stats.meshShaderSupport, &m_opaqueGeometryPipeline.handle,
            &m_postPassGeometryPipeline.handle, m_graphicsPipelineLayout.handle, 
            &m_onpcReflectiveGeometryPipeline.handle, m_onpcReflectiveGeometryLayout.handle))
        {
            BLIT_ERROR("Failed to create the primary graphics pipeline object");
            return 0;
        }

        // Updates the reference to the depth pyramid width held by the camera
        context.m_camera.viewData.pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
        context.m_camera.viewData.pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);

        return 1;
    }

    void VulkanRenderer::FinalSetup()
    {

    }
}