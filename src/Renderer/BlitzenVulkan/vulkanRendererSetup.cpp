#define VMA_IMPLEMENTATION// Implements vma funcions. Header file included in vulkanData.h
#include "vulkanResourceFunctions.h"
#include "vulkanCommands.h"
#include "vulkanRenderer.h"
#include "vulkanPipelines.h"
#include "vulkanRNDResources.h"

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

        uint32_t blockSize = (vulkanImageFormat == VK_FORMAT_BC1_RGBA_UNORM_BLOCK || vulkanImageFormat == VK_FORMAT_BC4_SNORM_BLOCK || 
            vulkanImageFormat == VK_FORMAT_BC4_UNORM_BLOCK) ? 8 : 16;

        size_t imageSize = BlitzenEngine::GetDDSImageSizeBC(header.dwWidth, header.dwHeight, header.dwMipMapCount, blockSize);
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
        // Staging buffer
        Buffer stagingBuffer;
        if(!CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, ce_textureStagingBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("Failed to create staging buffer for texture data copy");
            return 0;
        }

        void* pData{ stagingBuffer.m_vmaInfo.pMappedData };

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
        if(!Create2DTexture(stagingBuffer, m_device, m_allocator, m_readOnlies.m_textures[m_readOnlies.m_textureCount].image, {header.dwWidth, header.dwHeight}, 
            format, VK_IMAGE_USAGE_SAMPLED_BIT, m_commandsContext[0].m_transferCmdB, m_transferQueue.handle, header.dwMipMapCount))
        {
            BLIT_ERROR("Failed to load Vulkan texture image");
            return 0;
        }
        
        m_readOnlies.m_textures[m_readOnlies.m_textureCount].sampler = m_readOnlies.m_textureSampler.m_handle;
        m_readOnlies.m_textureCount++;
        return 1;
    }

    static VkDescriptorSetLayout CreateGPUBufferPushDescriptorBindings(VkDevice device, BlitCL::DynamicArray<VkDescriptorSetLayoutBinding>& bindings, uint8_t bRaytracing, uint8_t bMeshShaders)
    {
        auto viewDataShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT : 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        uint32_t bindingCount = 0;
        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_ViewDataBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, viewDataShaderStageFlags);

        auto vertexBufferShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT : VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_VertexBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vertexBufferShaderStageFlags);

        auto surfaceBufferShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_SurfaceBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, surfaceBufferShaderStageFlags);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_HI_Z_CullBinding, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_LODBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_TransformBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_MatBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_DrawCmdBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_DrawCmdCounterDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_DrawVisBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

        if (BlitzenCore::Ce_BuildClusters)
        {
            auto clusterBufferShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
                VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
            VkDescriptorSetLayoutBinding clusterBinding{};
            CreateDescriptorSetLayoutBinding(clusterBinding, Ce_ClusterBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, clusterBufferShaderStageFlags);

            bindings.PushBack(clusterBinding);
        }

        if (bRaytracing)
        {
            VkDescriptorSetLayoutBinding tlasBinding{};
            CreateDescriptorSetLayoutBinding(tlasBinding, Ce_TlasBufferBinding, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_FRAGMENT_BIT);

            bindings.PushBack(tlasBinding);
        }

        return CreateDescriptorSetLayout(device, (uint32_t)bindings.GetSize(), bindings.Data(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
    }

    static uint8_t CreateDescriptorLayouts(VkDevice device, DescriptorContext& descriptorContext, VulkanStats& stats, uint32_t textureCount)
    {
        // The big GPU push descriptor set layout. Holds most buffers
        BlitCL::DynamicArray<VkDescriptorSetLayoutBinding> pushDescriptorBindings{ Ce_DefaultPushDescriptorBindingCount, {} };
        
        descriptorContext.m_pushDescriptorLayout.handle = CreateGPUBufferPushDescriptorBindings(device, pushDescriptorBindings, stats.bRayTracingSupported, stats.meshShaderSupport);
        if (descriptorContext.m_pushDescriptorLayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create GPU buffer push descriptor layout");
            return 0;
        }

        // Descriptor set layout for textures
        VkDescriptorSetLayoutBinding texturesLayoutBinding{};
        CreateDescriptorSetLayoutBinding(texturesLayoutBinding, Ce_TextureDescriptorsBinding, textureCount, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT);
        descriptorContext.m_textureDescriptorSetlayout.handle = CreateDescriptorSetLayout(device, 1, &texturesLayoutBinding);
        if (descriptorContext.m_textureDescriptorSetlayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create texture descriptor set layout");
            return 0;
        }

        VkDescriptorSetLayoutBinding depthPyramidBindings [Ce_HI_Z_DescriptorCount] {};
        CreateDescriptorSetLayoutBinding(depthPyramidBindings[0], Ce_HI_Z_DstImageBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        CreateDescriptorSetLayoutBinding(depthPyramidBindings[1], Ce_HI_Z_SrcImageBinding, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
        descriptorContext.m_HI_Z_descriptorSetLayout.handle = CreateDescriptorSetLayout(device, Ce_HI_Z_DescriptorCount, depthPyramidBindings, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if (descriptorContext.m_HI_Z_descriptorSetLayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create depth pyramid descriptor set layout");
            return 0;
        }

        VkDescriptorSetLayoutBinding presentGenerationBindings[2]{};
        CreateDescriptorSetLayoutBinding(presentGenerationBindings[0], Ce_SwapchainDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        CreateDescriptorSetLayoutBinding(presentGenerationBindings[1], Ce_ColorTargetDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

        descriptorContext.m_presentSetlayout.handle = CreateDescriptorSetLayout(device, 2, presentGenerationBindings, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if (descriptorContext.m_presentSetlayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create present image generation layout");
            return 0;
        }

        // Success 
        return 1;
    }

    static uint8_t CreateReadWriteBuffers(VkDevice device, VmaAllocator vma, VkCommandBuffer commandBuffer, VkQueue queue, BlitzenEngine::DrawContext& context, 
        RWResources* readWritesArray, DescriptorContext& descriptorContext)
    {
        VK_CPU_DATA_BUFFER_SIZE_INFO transformBufferSizeInfo{};
        transformBufferSizeInfo.m_fullSSBOSize = BlitzenCore::Ce_MaxWorldTransformCount;
        transformBufferSizeInfo.m_dynamicDataSize = context.m_pResidents->m_transforms.m_dynamicTransformCount;
        transformBufferSizeInfo.m_dynamicDataOffset = BlitzenEngine::CE_DYNAMIC_TRANSFORM_OFFSET;
        transformBufferSizeInfo.m_staticDataSize = context.m_pResidents->m_transforms.m_staticTransformCount;
        transformBufferSizeInfo.m_staticDataOffset = BlitzenEngine::CE_STATIC_TRANSFORM_OFFSET;

        for (size_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            auto& readWrites = readWritesArray[frame];

            // Creates the uniform buffer for view data
            if (!CreateUBUFFER(vma, device, readWrites.m_viewDataBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
            {
                BLIT_ERROR("Failed to create view data buffer");
                return 0;
            }

            // Transform buffer is also dynamic
            Buffer transformStagingBufferTemp;
            auto transformBufferSize{CreateCPU_DATA_SSBO(vma, device, context.m_pResidents->m_transforms.m_transforms, readWrites.m_transformBuffer,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, transformStagingBufferTemp, transformBufferSizeInfo)};
            if (transformBufferSize == 0)
            {
                BLIT_ERROR("Failed to create transform buffer");
                return 0;
            }

            if (!CreateBuffer(vma, readWrites.m_drawCmdBuffer.m_buffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
                Ce_DrawCmdElementCount * sizeof(IndirectDrawData), 0))
            {
                BLIT_ERROR("Failed to create indirect draw cmd buffer");
                return 0;
            }

            if (!CreateBuffer(vma, readWrites.m_drawCmdCounterBuffer.m_buffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, sizeof(uint32_t), 0))
            {
                BLIT_ERROR("Failed to create indirect draw cmd counter buffer");
                return 0;
            }

            if (!CreateBuffer(vma, readWrites.m_drawVisBuffer.m_buffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
                context.m_pResidents->m_renders.m_renderCount * sizeof(uint32_t), 0))
            {
                BLIT_ERROR("Failed to create draw visibility buffer");
                return 0;
            }

            if (BlitzenCore::Ce_BuildClusters)
            {
                if (!CreateBuffer(vma, readWrites.m_clusterGroupDataBuffer.m_buffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
                    Ce_ClusterGroupBufferSize * sizeof(ClusterGroupData), 0))
                {
                    BLIT_ERROR("Failed to create cluster group buffer");
                    return 0;
                }
                descriptorContext.m_clusterGroupAddr[frame] = GetBufferAddress(device, readWrites.m_clusterGroupDataBuffer.m_buffer.m_handle);

                if (!CreateBuffer(vma, readWrites.m_clusterDispatchCounterBuffer.m_buffer, 
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT))
                {
                    BLIT_ERROR("Failed to create cluster dispatch counter");
                    return 0;
                }
                descriptorContext.m_clusterCounterAddr[frame] = GetBufferAddress(device, readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle);

                if (!CreateBuffer(vma, readWrites.m_clusterDispatchCounterCopy.m_buffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY, sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT))
                {
                    BLIT_ERROR("Failed to create cluster counter copy");
                    return 0;
                }

                if (context.m_pResidents->m_renders.m_transparentRenderCount != 0)
                {
                    if (!CreateBuffer(vma, readWrites.m_transClusterGroupDataBuffer.m_buffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY,
                        Ce_TransClusterGouprBufferSize * sizeof(ClusterGroupData), 0))
                    {
                        BLIT_ERROR("Failed to create transparent cluster group buffer");
                        return 0;
                    }
                    descriptorContext.m_transClusterGroupAddr[frame] = GetBufferAddress(device, readWrites.m_transClusterGroupDataBuffer.m_buffer.m_handle);

                    if (!CreateBuffer(vma, readWrites.m_transClusterDispatchCounterBuffer.m_buffer, 
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT))
                    {
                        BLIT_ERROR("Failed to create cluster dispatch counter");
                        return 0;
                    }
                    descriptorContext.m_transClusterCounterAddr[frame] = GetBufferAddress(device, readWrites.m_transClusterDispatchCounterBuffer.m_buffer.m_handle);

                    if (!CreateBuffer(vma, readWrites.m_transClusterDispatchCounterCopy.m_buffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(uint32_t),
                        VMA_ALLOCATION_CREATE_MAPPED_BIT))
                    {
                        BLIT_ERROR("Failed to create cluster counter copy buffer");
                        return 0;
                    }
                }
            }
            
            // Records command to copy staging buffer data to GPU buffers
            BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            CopyBufferToBuffer(commandBuffer, transformStagingBufferTemp.m_handle, readWrites.m_transformBuffer.m_buffer.m_handle, 
                transformBufferSizeInfo.m_staticDataSize * sizeof(BlitzenEngine::MeshTransform), 0, BlitzenEngine::CE_STATIC_TRANSFORM_OFFSET * sizeof(BlitzenEngine::MeshTransform));

            CopyBufferToBuffer(commandBuffer, readWrites.m_transformBuffer.m_staging.m_handle, readWrites.m_transformBuffer.m_buffer.m_handle, 
                readWrites.m_transformBuffer.m_copyDataSize, 0, BlitzenEngine::CE_DYNAMIC_TRANSFORM_OFFSET * sizeof(BlitzenEngine::MeshTransform));

            vkCmdFillBuffer(commandBuffer, readWrites.m_drawVisBuffer.m_buffer.m_handle, 0, context.m_pResidents->m_renders.m_renderCount * sizeof(uint32_t), 0);

            SubmitCommandBuffer(queue, commandBuffer, 0, nullptr, 0, nullptr, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);
        }

        return 1;
    }

    static uint8_t CreateReadOnlyBuffers(VkInstance instance, VkDevice device, VmaAllocator vma, CommandContext& cmdContext, VkQueue queue, ROResources& readOnlies, 
        BlitzenEngine::DrawContext& context, VulkanStats& stats, DescriptorContext& descriptorContext)
    {
        // Additional RT flags for geometry
        auto bRT{ stats.bRayTracingSupported };
        uint32_t geometryRtFlags = bRT ? VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0;

        // Vertex buffer
        Buffer vtxStaging{};
        VkDeviceSize vertexBufferSize{ CreateSSBO(vma, device, context.m_meshes.m_vertices.Data(), readOnlies.m_vtxBuffer, vtxStaging,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | geometryRtFlags, (uint32_t)context.m_meshes.m_vertices.GetSize())};
        if (vertexBufferSize == 0)
        {
            BLIT_ERROR("Failed to create vertex buffer");
            return 0;
        }

        // Index buffer
        Buffer idxStaging;
        VkDeviceSize indexBufferSize{ CreateSSBO(vma, device, context.m_meshes.m_indices.Data(), readOnlies.m_idxBuffer, idxStaging,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, (uint32_t)context.m_meshes.m_indices.GetSize()) };
        if(indexBufferSize == 0)
        {
            BLIT_ERROR("Failed to create index buffer");
            return 0;
        }

        // Opaque render buffer
        Buffer renderStaging;
        VkDeviceSize renderBufferSize{ CreateSSBO(vma, device, context.m_pResidents->m_renders.m_renders, readOnlies.m_renderBuffer, renderStaging,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, context.m_pResidents->m_renders.m_renderCount)};
        if (renderBufferSize == 0)
        {
            BLIT_ERROR("Failed to create render object buffer");
            return 0;
        }
        // Address for push constant
        descriptorContext.m_opaqueRenderAddr= GetBufferAddress(device, readOnlies.m_renderBuffer.m_buffer.m_handle);

        // Surface buffer
        Buffer surfaceStaging;
        VkDeviceSize surfaceBufferSize{ CreateSSBO(vma, device, context.m_meshes.m_surfaces.Data(), readOnlies.m_surfaceBuffer, surfaceStaging,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, (uint32_t)context.m_meshes.m_surfaces.GetSize())};
        if (surfaceBufferSize == 0)
        {
            BLIT_ERROR("Failed to create surface buffer");
            return 0;
        }

        // Lod buffer
        Buffer LODstaging;
        VkDeviceSize lodBufferSize{ CreateSSBO(vma, device, context.m_meshes.m_LODs.Data(), readOnlies.m_LODBuffer, LODstaging,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, (uint32_t)context.m_meshes.m_LODs.GetSize())};
        if (lodBufferSize == 0)
        {
            BLIT_ERROR("Failed to create surface buffer");
            return 0;
        }

        // Mat buffer
        Buffer matStaging;
        VkDeviceSize materialBufferSize{ CreateSSBO(vma, device, context.m_textures.m_materials, readOnlies.m_matBuffer, matStaging, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            context.m_textures.m_materialCount)};
        if (materialBufferSize == 0)
        {
            BLIT_ERROR("Failed to create material buffer");
            return 0;
        }

        // Cluster mode buffers
        VkDeviceSize clusterBufferSize = 0;
        Buffer clusterStagingBuffer;
        VkDeviceSize clusterIndexBufferSize = 0;
        Buffer clusterIndexStagingBuffer;
        if (BlitzenCore::Ce_BuildClusters)
        {
            clusterBufferSize = CreateSSBO(vma, device, context.m_meshes.m_clusters.Data(), readOnlies.m_clusterBuffer, clusterStagingBuffer,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, (uint32_t)context.m_meshes.m_clusters.GetSize());
            if (clusterBufferSize == 0)
            {
                BLIT_ERROR("Failed to create cluster buffer");
                return 0;
            }

            clusterIndexBufferSize = CreateSSBO(vma, device, context.m_meshes.m_clusterIndices.Data(), readOnlies.m_clusterIdxBuffer, clusterIndexStagingBuffer,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, (uint32_t)context.m_meshes.m_clusterIndices.GetSize());
            if (clusterIndexBufferSize == 0)
            {
                BLIT_ERROR("Failed to create cluster indices buffer");
                return 0;
            }
        }

        BeginCommandBuffer(cmdContext.m_transferCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, vtxStaging.m_handle, readOnlies.m_vtxBuffer.m_buffer.m_handle, vertexBufferSize, 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, idxStaging.m_handle, readOnlies.m_idxBuffer.m_buffer.m_handle, indexBufferSize, 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, renderStaging.m_handle, readOnlies.m_renderBuffer.m_buffer.m_handle, renderBufferSize, 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, surfaceStaging.m_handle, readOnlies.m_surfaceBuffer.m_buffer.m_handle, surfaceBufferSize, 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, LODstaging.m_handle, readOnlies.m_LODBuffer.m_buffer.m_handle, lodBufferSize, 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, matStaging.m_handle, readOnlies.m_matBuffer.m_buffer.m_handle, materialBufferSize, 0, 0);

        if (BlitzenCore::Ce_BuildClusters)
        {
            CopyBufferToBuffer(cmdContext.m_transferCmdB, clusterStagingBuffer.m_handle, readOnlies.m_clusterBuffer.m_buffer.m_handle, clusterBufferSize, 0, 0);

            CopyBufferToBuffer(cmdContext.m_transferCmdB, clusterIndexStagingBuffer.m_handle, readOnlies.m_clusterIdxBuffer.m_buffer.m_handle, clusterIndexBufferSize, 0, 0);
        }

        // Submit the commands and wait for the queue to finish
        SubmitCommandBuffer(queue, cmdContext.m_transferCmdB, 0, nullptr, 0, nullptr, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        // Raytracing
        if (stats.bRayTracingSupported)
        {
            if (!BuildBlas(instance, device, vma, cmdContext, queue, context, readOnlies))
            {
                BLIT_ERROR("Failed to build blas for RT");
                return 0;
            }

            if (!BuildTlas(instance, device, vma, cmdContext, queue, readOnlies, context))
            {
                BLIT_ERROR("Failed to build tlas for RT");
                return 0;
            }
        }

        // SUCCESS
        return 1;
    }

    static void CreateDescriptors(DescriptorContext& descriptorContext, ROResources& roResources, RWResources* rwResourcesArray, BlitzenEngine::DrawContext& drawContext)
    {
        descriptorContext.m_vtxDescInfo.buffer = roResources.m_vtxBuffer.m_buffer.m_handle;
        descriptorContext.m_vtxDescInfo.offset = 0;
        descriptorContext.m_vtxDescInfo.range = drawContext.m_meshes.m_vertices.GetSize() * sizeof(BlitzenEngine::Vertex);
        WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsGraphics[Ce_VertexBufferGraphicsPushID], &descriptorContext.m_vtxDescInfo,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_VertexBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

        descriptorContext.m_matDescInfo.buffer = roResources.m_matBuffer.m_buffer.m_handle;
        descriptorContext.m_matDescInfo.offset = 0;
        descriptorContext.m_matDescInfo.range = drawContext.m_textures.m_materialCount * sizeof(BlitzenEngine::Material);
        WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsGraphics[Ce_MatBufferGraphicsPushID], &descriptorContext.m_matDescInfo,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_MatBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

        for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            auto& rw{ rwResourcesArray[frame] };

            descriptorContext.m_viewDescInfo[frame].buffer = rw.m_viewDataBuffer.m_buffer.m_handle;
            descriptorContext.m_viewDescInfo[frame].offset = 0;
            descriptorContext.m_viewDescInfo[frame].range = sizeof(BlitzenEngine::CameraViewData);
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsShared[Ce_ViewDataBufferSharedPushID + frame * Ce_SharedDescriptorCount], &descriptorContext.m_viewDescInfo[frame], 
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, Ce_ViewDataBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

            descriptorContext.m_surfaceDescInfo[frame].buffer = roResources.m_surfaceBuffer.m_buffer.m_handle;
            descriptorContext.m_surfaceDescInfo[frame].offset = 0;
            descriptorContext.m_surfaceDescInfo[frame].range = drawContext.m_meshes.m_surfaces.GetSize() * sizeof(BlitzenEngine::PrimitiveSurface);
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsShared[Ce_SurfaceBufferSharedPushID + frame * Ce_SharedDescriptorCount], &descriptorContext.m_surfaceDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_SurfaceBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

            descriptorContext.m_transformDescInfo[frame].buffer = rw.m_transformBuffer.m_buffer.m_handle;
            descriptorContext.m_transformDescInfo[frame].offset = 0;
            descriptorContext.m_transformDescInfo[frame].range = (drawContext.m_pResidents->m_transforms.m_staticTransformCount + BlitzenCore::Ce_MaxDynamicObjectCount) * sizeof(BlitzenEngine::MeshTransform);
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsShared[Ce_TransformBufferSharedPushID + frame * Ce_SharedDescriptorCount], &descriptorContext.m_transformDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_TransformBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

            descriptorContext.m_drawCmdDescInfo[frame].buffer = rw.m_drawCmdBuffer.m_buffer.m_handle;
            descriptorContext.m_drawCmdDescInfo[frame].offset = 0;
            descriptorContext.m_drawCmdDescInfo[frame].range = Ce_DrawCmdElementCount * sizeof(IndirectDrawData);
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsShared[Ce_DrawCmdBufferSharedPushID + frame * Ce_SharedDescriptorCount], &descriptorContext.m_drawCmdDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_DrawCmdBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);
        }

        for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            auto& rw{ rwResourcesArray[frame] };

            descriptorContext.m_LODDescInfo[frame].buffer = roResources.m_LODBuffer.m_buffer.m_handle;
            descriptorContext.m_LODDescInfo[frame].offset = 0;
            descriptorContext.m_LODDescInfo[frame].range = VK_WHOLE_SIZE;
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsCull[Ce_LODBufferCullPushID + frame * Ce_CullDescriptorCount], &descriptorContext.m_LODDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_LODBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

            descriptorContext.m_drawCmdCounterDescInfo[frame].buffer = rw.m_drawCmdCounterBuffer.m_buffer.m_handle;
            descriptorContext.m_drawCmdCounterDescInfo[frame].offset = 0;
            descriptorContext.m_drawCmdCounterDescInfo[frame].range = VK_WHOLE_SIZE;
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsCull[Ce_DrawCmdCounterCullPushID + frame * Ce_CullDescriptorCount], &descriptorContext.m_drawCmdCounterDescInfo[frame], 
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_DrawCmdCounterDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);
        }

        for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            auto& rw{ rwResourcesArray[frame] };

            descriptorContext.m_drawVisDescInfo[frame].buffer = rw.m_drawVisBuffer.m_buffer.m_handle;
            descriptorContext.m_drawVisDescInfo[frame].offset = 0;
            descriptorContext.m_drawVisDescInfo[frame].range = VK_WHOLE_SIZE;
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsDrawOcc[Ce_DrawVisBufferOccPushID + frame * Ce_DrawOcclusionDescriptorCount], &descriptorContext.m_drawVisDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_DrawVisBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);
        }

        descriptorContext.m_clusterBufferDescInfo.buffer = roResources.m_clusterBuffer.m_buffer.m_handle;
        descriptorContext.m_clusterBufferDescInfo.offset = 0;
        descriptorContext.m_clusterBufferDescInfo.range = drawContext.m_meshes.m_clusters.GetSize() * sizeof(BlitzenEngine::Cluster);
        WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsClusterCull[Ce_ClusterBufferPushID], &descriptorContext.m_clusterBufferDescInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            Ce_ClusterBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);
    }

    static uint8_t CreatePipelineLayouts(VkDevice device, PipelineContext& context, DescriptorContext& descriptorContext)
    {
        // OPAQUE GRAPHICS
        VkDescriptorSetLayout opaqueDrawDescLayouts[2] = { descriptorContext.m_pushDescriptorLayout.handle, descriptorContext.m_textureDescriptorSetlayout.handle};

        VkPushConstantRange globalShaderDataPushContant{};
        CreatePushConstantRange(globalShaderDataPushContant, VK_SHADER_STAGE_VERTEX_BIT, sizeof(GlobalShaderDataPushConstant));

        if (!CreatePipelineLayout(device, &context.m_opaqueDrawLayout.handle, BLIT_ARRAY_SIZE(opaqueDrawDescLayouts), opaqueDrawDescLayouts, 1, &globalShaderDataPushContant))
        {
            BLIT_ERROR("Failed to create main graphics pipeline layout");
            return 0;
        }
        
        // CULLING SHADERS
        VkPushConstantRange cullShaderPushConstant{};
        CreatePushConstantRange(cullShaderPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(DrawCullShaderPushConstant));

        if (!CreatePipelineLayout(device, &context.m_drawCullLayout.handle, 1, &descriptorContext.m_pushDescriptorLayout.handle, 1, &cullShaderPushConstant))
        {
            BLIT_ERROR("Failed to create culling pipeline layout");
            return 0;
        }

        // CLUSTER CULLING SHADERS
        if (BlitzenCore::Ce_BuildClusters)
        {
            VkPushConstantRange clusterCullPushConstant{};
            CreatePushConstantRange(clusterCullPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(ClusterCullShaderPushConstant));
            if (!CreatePipelineLayout(device, &context.m_clusterCullLayout.handle, 1, &descriptorContext.m_pushDescriptorLayout.handle, 1, &clusterCullPushConstant))
            {
                BLIT_ERROR("Failed to create culling pipeline layout");
                return 0;
            }
        }

        // HI Z SHADER
        VkPushConstantRange HI_Z_pushConstant{};
        CreatePushConstantRange(HI_Z_pushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(BlitML::vec2));

        if (!CreatePipelineLayout(device, &context.m_hiZLayout.handle, 1, &descriptorContext.m_HI_Z_descriptorSetLayout.handle, 1, &HI_Z_pushConstant))
        {
            BLIT_ERROR("Failed to create depth pyramid generation pipeline layout");
            return 0;
        }

        // Generate present image compute shader layout
        VkPushConstantRange presentPushConstant{};
        CreatePushConstantRange(presentPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(BlitML::vec2));

        if (!CreatePipelineLayout(device, &context.m_presentLayout.handle, 1, &descriptorContext.m_presentSetlayout.handle, 1, &presentPushConstant))
        {
            BLIT_ERROR("Failed to create presentation generation pipeline layout");
            return 0;
        }

        // Success
        return 1;
    }

    static uint8_t AllocateTextureDescriptorSet(VkDevice device, ROResources& readOnlies, DescriptorContext& descriptorContext)
    {
        if (readOnlies.m_textureCount == 0)
        {
            BLIT_ERROR("No textures loaded");
            return 0;
        }

        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = BlitzenCore::Ce_MaxTextureCount;

        descriptorContext.m_textureDescriptorPool.handle = CreateDescriptorPool(device, 1, &poolSize, 1);
        if (descriptorContext.m_textureDescriptorPool.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create descriptor pool for textures");
            return 0;
        }

        if (!AllocateDescriptorSets(device, descriptorContext.m_textureDescriptorPool.handle, &descriptorContext.m_textureDescriptorSetlayout.handle, 1, &descriptorContext.m_textureDescriptorSet))
        {
            BLIT_ERROR("Failed to allocate descriptor set for textures");
            return 0;
        }

        // Array of descriptor infos
        BlitCL::DynamicArray<VkDescriptorImageInfo> imageInfos{ readOnlies.m_textureCount };
        for (size_t i = 0; i < imageInfos.GetSize(); ++i)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = readOnlies.m_textures[i].image.m_view.m_handle;
            imageInfos[i].sampler = readOnlies.m_textures[i].sampler;
        }

        VkWriteDescriptorSet write{};
        WriteImageDescriptorSets(write, imageInfos.Data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorContext.m_textureDescriptorSet, uint32_t(imageInfos.GetSize()), 
            Ce_TextureDescriptorsBinding);
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

        return 1;
    }


    uint8_t VulkanRenderer::SetupForRendering(BlitzenEngine::DrawContext& context)
    {
        if(!CreateDescriptorLayouts(m_device, m_descriptorContext, m_stats, m_readOnlies.m_textureCount))
        {
            BLIT_ERROR("Failed to create descriptor set layouts");
            return 0;
        }

        if (!CreatePipelineLayouts(m_device, m_pipelines, m_descriptorContext))
        {
            BLIT_ERROR("Failed to create pipeline layouts");
            return 0;
        }

        if (!CreateReadOnlyBuffers(m_instance, m_device, m_allocator, m_commandsContext[0], m_transferQueue.handle, m_readOnlies, context, m_stats, m_descriptorContext))
        {
            BLIT_ERROR("Failed to create read only buffers");
            return 0;
        }

        if(!CreateReadWriteBuffers(m_device, m_allocator, m_commandsContext[0].m_transferCmdB, m_transferQueue.handle, context, m_readWrites, m_descriptorContext))
        {
            BLIT_ERROR("Failed to create read write buffers");
            return 0;
        }

        if (!AllocateTextureDescriptorSet(m_device, m_readOnlies, m_descriptorContext))
        {
            BLIT_ERROR("Failed to allocate texture descriptor sets");
            return 0;
        }

        if (!CreateComputeShaders(m_device, m_pipelines))
        {
            BLIT_ERROR("Failed to create compute shaders");
            return 0;
        }
        
        // Create the graphics pipeline object 
        if(!CreateGraphicsPipelines(m_device, m_stats.meshShaderSupport, m_pipelines))
        {
            BLIT_ERROR("Failed to create the primary graphics pipeline object");
            return 0;
        }

        CreateDescriptors(m_descriptorContext, m_readOnlies, m_readWrites, context);

        // Updates the reference to the depth pyramid width held by the camera
        context.m_camera.viewData.pyramidWidth = float(m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_width);
        context.m_camera.viewData.pyramidHeight = float(m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_height);

        return 1;
    }

    void VulkanRenderer::FinalSetup()
    {
        vkDeviceWaitIdle(m_device);
    }
}