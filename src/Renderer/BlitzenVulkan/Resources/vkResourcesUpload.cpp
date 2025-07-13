#define VMA_IMPLEMENTATION// Implements vma funcions. Header file included in vulkanData.h
#include "vulkanResourceFunctions.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vulkanCommands.h"
#include "Renderer/BlitzenVulkan/Context/vulkanRenderer.h"
#include "vulkanPipelines.h"
#include "vulkanRNDResources.h"
#include "BlitCL/blitDynamicArr.h"
#include "Core/DbLog/blitLogger.h"

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

    uint8_t UploadResourcesToBuffers(VkDevice device, VkInstance instance, VmaAllocator vma, VkQueue queue, BlitzenEngine::DrawContext& drawContext, 
        ROResources& readOnlies, RWResources* pRWResourcesArray,CommandContext& cmdContext, VulkanStats& stats, LoadingContextMesh& loadingContextMesh)
    {
        for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            auto& readWrites{ pRWResourcesArray[frame] };
            auto commandBuffer{ cmdContext.m_transferCmdB };

            if (!drawContext.m_pResidents->m_transforms.m_transforms)
            {
                BLIT_ERROR("%s: No transform data found", BLIT_VK_SYSTEM);
                return 0;
            }

            BUFFER_STAGING_CONTEXT<BlitzenEngine::MeshTransform> transformStagingContext{};
            transformStagingContext.elementCount = BLIT_MAX_WORLD_TRANSFORM_COUNT;
            transformStagingContext.pData = &drawContext.m_pResidents->m_transforms.m_transforms[BlitzenEngine::CE_STATIC_TRANSFORM_OFFSET];
            if (!CreateStaging(vma, device, transformStagingContext))
            {
                BLIT_ERROR("%s: Failed to create static transform staging buffer resource");
                return 0;
            }

            // Records command to copy staging buffer data to GPU buffers
            BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            CopyBufferToBuffer(commandBuffer, transformStagingContext.staging.m_buffer.m_handle, readWrites.m_transformBuffer.m_buffer.m_handle,
                transformStagingContext.staging.m_dataSize, 0, 0);

            vkCmdFillBuffer(commandBuffer, readWrites.m_drawVisBuffer.m_buffer.m_handle, 0, drawContext.m_pResidents->m_renders.RENDER_COUNT * sizeof(uint32_t), 0);

            SubmitCommandBuffer(queue, commandBuffer, 0, nullptr, 0, nullptr, VK_NULL_HANDLE);
            vkQueueWaitIdle(queue);
        }

        BUFFER_STAGING_CONTEXT<BlitzenEngine::RenderObject> renderStagingContext{};
        renderStagingContext.elementCount = BLIT_MAX_WORLD_RENDERS;
        renderStagingContext.pData = drawContext.m_pResidents->m_renders.m_renders;
        if (!CreateStaging(vma, device, renderStagingContext))
        {
            BLIT_ERROR("%s: Failed to create render staging buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        BUFFER_STAGING_CONTEXT<BlitzenEngine::BoundingSphere> boundingSphereStagingContext{};
        boundingSphereStagingContext.elementCount = BLIT_MAX_WORLD_RENDERS;
        boundingSphereStagingContext.pData = drawContext.m_pResidents->MColliders.m_boundingSpheres;
        if (!CreateStaging(vma, device, boundingSphereStagingContext))
        {
            BLIT_ERROR("%s: Failed to create bounding sphere staging buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        BUFFER_STAGING_CONTEXT<BlitzenEngine::PrimitiveSurface> surfaceStagingContext{};
        surfaceStagingContext.elementCount = drawContext.m_meshes.m_meshPrimitives.m_meshPrimitivesCount;
        surfaceStagingContext.pData = drawContext.m_meshes.m_meshPrimitives.m_meshPrimitives;
        if (!CreateStaging(vma, device, surfaceStagingContext))
        {
            BLIT_ERROR("%s: Failed to create surface staging buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        BUFFER_STAGING_CONTEXT<BlitzenEngine::LodData> LODstagingContext{};
        LODstagingContext.elementCount = drawContext.m_meshes.m_meshPrimitives.m_LODCount;
        LODstagingContext.pData = drawContext.m_meshes.m_meshPrimitives.m_LODs;
        if (!CreateStaging(vma, device, LODstagingContext))
        {
            BLIT_ERROR("%s: Failed to create LOD staging buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        BUFFER_STAGING_CONTEXT<BlitzenEngine::Material> matStagingContext{};
        matStagingContext.elementCount = drawContext.m_textures.m_materialCount;
        matStagingContext.pData = drawContext.m_textures.m_materials;
        if (!CreateStaging(vma, device, matStagingContext))
        {
            BLIT_ERROR("%s: Failed to create material staging buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        BUFFER_STAGING_CONTEXT<BlitzenEngine::ClusterVertices> clusterVtxStagingContext{};
        BUFFER_STAGING_CONTEXT<BlitzenEngine::ClusterSphere> clusterSphereStagingContext{};
        BUFFER_STAGING_CONTEXT<BlitzenEngine::ClusterCone> clusterConesStagingContext{};
        BUFFER_STAGING_CONTEXT<uint32_t> clusterIndexStaging{};
        if constexpr (BlitzenCore::Ce_BuildClusters)
        {
            clusterVtxStagingContext.elementCount = drawContext.m_meshes.m_clusters.m_clusterCount;
            clusterVtxStagingContext.pData = drawContext.m_meshes.m_clusters.m_clusterVertices;
            if (!CreateStaging(vma, device, clusterVtxStagingContext))
            {
                BLIT_ERROR("%s: Failed to create cluster vertices staging buffer", BLIT_VK_SYSTEM);
                return 0;
            }

            clusterSphereStagingContext.elementCount = drawContext.m_meshes.m_clusters.m_clusterCount;
            clusterSphereStagingContext.pData = drawContext.m_meshes.m_clusters.m_clusterSpheres;
            if (!CreateStaging(vma, device, clusterSphereStagingContext))
            {
                BLIT_ERROR("%s: Failed to create cluster spheres staging buffer", BLIT_VK_SYSTEM);
                return 0;
            }

            clusterConesStagingContext.elementCount = drawContext.m_meshes.m_clusters.m_clusterCount;
            clusterConesStagingContext.pData = drawContext.m_meshes.m_clusters.m_clusterCones;
            if (!CreateStaging(vma, device, clusterConesStagingContext))
            {
                BLIT_ERROR("%s: Failed to create cluster cones staging buffer", BLIT_VK_SYSTEM);
                return 0;
            }

            clusterIndexStaging.elementCount = drawContext.m_meshes.m_clusters.m_clusterIndicesCount;
            clusterIndexStaging.pData = drawContext.m_meshes.m_clusters.m_clusterIndices;
            if (!CreateStaging(vma, device, clusterIndexStaging))
            {
                BLIT_ERROR("Failed to create cluster index staging buffer", BLIT_VK_SYSTEM);
                return 0;
            }
        }

        BeginCommandBuffer(cmdContext.m_transferCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, loadingContextMesh.m_vtxPosStaging.m_buffer.m_handle, readOnlies.m_vtxPosBuffer.m_buffer.m_handle, 
            loadingContextMesh.m_vtxPosStaging.m_validIndex * sizeof(BlitzenEngine::VtxPos), 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, loadingContextMesh.m_vtxNrmStaging.m_buffer.m_handle, readOnlies.m_vtxNrmBuffer.m_buffer.m_handle,
            loadingContextMesh.m_vtxNrmStaging.m_validIndex * sizeof(BlitzenEngine::VtxNormals), 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, loadingContextMesh.m_vtxTngStaging.m_buffer.m_handle, readOnlies.m_vtxTngBuffer.m_buffer.m_handle,
            loadingContextMesh.m_vtxTngStaging.m_validIndex * sizeof(BlitzenEngine::VtxTangents), 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, loadingContextMesh.m_vtxTexCoordStaging.m_buffer.m_handle, readOnlies.m_vtxTexCoordBuffer.m_buffer.m_handle,
            loadingContextMesh.m_vtxTexCoordStaging.m_validIndex * sizeof(BlitzenEngine::VtxTexCoords), 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, loadingContextMesh.m_vtxIdxStaging.m_buffer.m_handle, readOnlies.m_idxBuffer.m_buffer.m_handle, 
            loadingContextMesh.m_vtxIdxStaging.m_validIndex * sizeof(uint32_t), 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, renderStagingContext.staging.m_buffer.m_handle, readOnlies.m_renderBuffer.m_buffer.m_handle, renderStagingContext.staging.m_dataSize, 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, surfaceStagingContext.staging.m_buffer.m_handle, readOnlies.m_surfaceBuffer.m_buffer.m_handle, surfaceStagingContext.staging.m_dataSize, 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, LODstagingContext.staging.m_buffer.m_handle, readOnlies.m_LODBuffer.m_buffer.m_handle, LODstagingContext.staging.m_dataSize, 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, matStagingContext.staging.m_buffer.m_handle, readOnlies.m_matBuffer.m_buffer.m_handle, matStagingContext.staging.m_dataSize, 0, 0);

        CopyBufferToBuffer(cmdContext.m_transferCmdB, boundingSphereStagingContext.staging.m_buffer.m_handle, readOnlies.m_boundingSphereBuffer.m_buffer.m_handle, boundingSphereStagingContext.staging.m_dataSize, 0, 0);

        if (BlitzenCore::Ce_BuildClusters)
        {
            CopyBufferToBuffer(cmdContext.m_transferCmdB, clusterVtxStagingContext.staging.m_buffer.m_handle, readOnlies.m_clusterVtxsBuffer.m_buffer.m_handle, 
                clusterVtxStagingContext.staging.m_dataSize, 0, 0);

            CopyBufferToBuffer(cmdContext.m_transferCmdB, clusterSphereStagingContext.staging.m_buffer.m_handle, readOnlies.m_clusterSpheresBuffer.m_buffer.m_handle,
                clusterSphereStagingContext.staging.m_dataSize, 0, 0);

            CopyBufferToBuffer(cmdContext.m_transferCmdB, clusterConesStagingContext.staging.m_buffer.m_handle, readOnlies.m_clusterConesBuffer.m_buffer.m_handle,
                clusterConesStagingContext.staging.m_dataSize, 0, 0);

            CopyBufferToBuffer(cmdContext.m_transferCmdB, clusterIndexStaging.staging.m_buffer.m_handle, readOnlies.m_clusterIdxBuffer.m_buffer.m_handle, clusterIndexStaging.staging.m_dataSize, 0, 0);
        }

        // Submit the commands and wait for the queue to finish
        SubmitCommandBuffer(queue, cmdContext.m_transferCmdB, 0, nullptr, 0, nullptr, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        // Raytracing
        if (stats.bRayTracingSupported)
        {
            if (!BuildBlas(instance, device, vma, cmdContext, queue, drawContext, readOnlies))
            {
                BLIT_ERROR("Failed to build blas for RT");
                return 0;
            }

            if (!BuildTlas(instance, device, vma, cmdContext, queue, readOnlies, drawContext))
            {
                BLIT_ERROR("Failed to build tlas for RT");
                return 0;
            }
        }

        return 1;
    }

    void CreateDescriptors(DescriptorContext& descriptorContext, ROResources& roResources, RWResources* rwResourcesArray, BlitzenEngine::DrawContext& drawContext)
    {
        descriptorContext.m_vtxPosDescInfo.buffer = roResources.m_vtxPosBuffer.m_buffer.m_handle;
        descriptorContext.m_vtxPosDescInfo.offset = 0;
        descriptorContext.m_vtxPosDescInfo.range = drawContext.m_meshes.m_triangles.m_vertexCount * sizeof(BlitzenEngine::VtxPos);
        WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsGraphics[Ce_VertexPosBufferGraphicsPushID], &descriptorContext.m_vtxPosDescInfo,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_VertexBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

        descriptorContext.m_vtxNrmDescInfo.buffer = roResources.m_vtxNrmBuffer.m_buffer.m_handle;
        descriptorContext.m_vtxNrmDescInfo.offset = 0;
        descriptorContext.m_vtxNrmDescInfo.range = drawContext.m_meshes.m_triangles.m_vertexCount * sizeof(BlitzenEngine::VtxNormals);
        WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsGraphics[Ce_VertexNrmBufferGraphicsPushID], &descriptorContext.m_vtxNrmDescInfo,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_VertexNrmBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

        descriptorContext.m_vtxTngDescInfo.buffer = roResources.m_vtxTngBuffer.m_buffer.m_handle;
        descriptorContext.m_vtxTngDescInfo.offset = 0;
        descriptorContext.m_vtxTngDescInfo.range = drawContext.m_meshes.m_triangles.m_vertexCount * sizeof(BlitzenEngine::VtxTangents);
        WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsGraphics[Ce_VertexTngBufferGraphicsPushID], &descriptorContext.m_vtxTngDescInfo,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_VertexTngBufferDescriptorBidning, nullptr, VK_NULL_HANDLE, 1, 0);

        descriptorContext.m_vtxTexCoordsInfo.buffer = roResources.m_vtxTexCoordBuffer.m_buffer.m_handle;
        descriptorContext.m_vtxTexCoordsInfo.offset = 0;
        descriptorContext.m_vtxTexCoordsInfo.range = drawContext.m_meshes.m_triangles.m_vertexCount * sizeof(BlitzenEngine::VtxTexCoords);
        WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsGraphics[Ce_VertexTexCoordsBufferGraphicsPushID], &descriptorContext.m_vtxTexCoordsInfo,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_VertexTexCoordBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

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
            descriptorContext.m_surfaceDescInfo[frame].range = drawContext.m_meshes.m_meshPrimitives.m_meshPrimitivesCount * sizeof(BlitzenEngine::PrimitiveSurface);
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsShared[Ce_SurfaceBufferSharedPushID + frame * Ce_SharedDescriptorCount], &descriptorContext.m_surfaceDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_SurfaceBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

            descriptorContext.m_transformDescInfo[frame].buffer = rw.m_transformBuffer.m_buffer.m_handle;
            descriptorContext.m_transformDescInfo[frame].offset = 0;
            descriptorContext.m_transformDescInfo[frame].range = BLIT_MAX_WORLD_TRANSFORM_COUNT * sizeof(BlitzenEngine::MeshTransform);
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsShared[Ce_TransformBufferSharedPushID + frame * Ce_SharedDescriptorCount], &descriptorContext.m_transformDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_TransformBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

            descriptorContext.m_drawCmdDescInfo[frame].buffer = rw.m_staticDrawCmdBuffer.m_buffer.m_handle;
            descriptorContext.m_drawCmdDescInfo[frame].offset = 0;
            descriptorContext.m_drawCmdDescInfo[frame].range = Ce_DrawCmdElementCount * sizeof(IndirectDrawData);
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsShared[Ce_DrawCmdBufferSharedPushID + frame * Ce_SharedDescriptorCount], &descriptorContext.m_drawCmdDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_DrawCmdBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

            descriptorContext.m_renderBufferDescInfo[frame].buffer = roResources.m_renderBuffer.m_buffer.m_handle;
            descriptorContext.m_renderBufferDescInfo[frame].offset = 0;
            descriptorContext.m_renderBufferDescInfo[frame].range = BLIT_MAX_WORLD_RENDERS * sizeof(BlitzenEngine::RenderObject);
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsShared[Ce_RenderBufferSharedPushID + frame * Ce_SharedDescriptorCount], &descriptorContext.m_renderBufferDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_RenderBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);
        }

        for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            auto& rw{ rwResourcesArray[frame] };

            descriptorContext.m_LODDescInfo[frame].buffer = roResources.m_LODBuffer.m_buffer.m_handle;
            descriptorContext.m_LODDescInfo[frame].offset = 0;
            descriptorContext.m_LODDescInfo[frame].range = VK_WHOLE_SIZE;
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsCull[Ce_LODBufferCullPushID + frame * Ce_CullDescriptorCount], &descriptorContext.m_LODDescInfo[frame],
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_LODBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

            descriptorContext.m_drawCmdCounterDescInfo[frame].buffer = rw.m_staticDrawCmdCount.m_buffer.m_handle;
            descriptorContext.m_drawCmdCounterDescInfo[frame].offset = 0;
            descriptorContext.m_drawCmdCounterDescInfo[frame].range = VK_WHOLE_SIZE;
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsCull[Ce_DrawCmdCounterCullPushID + frame * Ce_CullDescriptorCount], &descriptorContext.m_drawCmdCounterDescInfo[frame], 
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_DrawCmdCounterDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

            descriptorContext.m_boundingSphereDescInfo[frame].buffer = roResources.m_boundingSphereBuffer.m_buffer.m_handle;
            descriptorContext.m_boundingSphereDescInfo[frame].offset = 0;
            descriptorContext.m_boundingSphereDescInfo[frame].range = BLIT_MAX_WORLD_RENDERS * sizeof(BlitzenEngine::BoundingSphere);
            WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsCull[Ce_BoundingSphereCullPushID + frame * Ce_CullDescriptorCount], &descriptorContext.m_boundingSphereDescInfo[frame], 
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_BoundingSphereDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);
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

        if constexpr (BlitzenCore::Ce_BuildClusters)
        {
            for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
            {
                auto& rw{ rwResourcesArray[frame] };

                descriptorContext.m_clusterBufferDescInfo.buffer = roResources.m_clusterVtxsBuffer.m_buffer.m_handle;
                descriptorContext.m_clusterBufferDescInfo.offset = 0;
                descriptorContext.m_clusterBufferDescInfo.range = drawContext.m_meshes.m_clusters.m_clusterCount * sizeof(BlitzenEngine::ClusterVertices);
                WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsClusterCull[Ce_ClusterBufferPushID + frame * Ce_ClusterCullDescriptorCount], 
                    &descriptorContext.m_clusterBufferDescInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_ClusterBufferDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);

                descriptorContext.m_clusterGroupDescInfo.buffer = rw.m_clusterGroupDataBuffer.m_buffer.m_handle;
                descriptorContext.m_clusterGroupDescInfo.offset = 0;
                descriptorContext.m_clusterGroupDescInfo.range = Ce_ClusterGroupBufferSize * sizeof(ClusterGroupData);
                WriteBufferDescriptorSets(descriptorContext.m_pushDescriptorsClusterCull[Ce_ClusterGroupBufferPushID + frame * Ce_ClusterCullDescriptorCount],
                    &descriptorContext.m_clusterGroupDescInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, Ce_ClusterGroupDescriptorBinding, nullptr, VK_NULL_HANDLE, 1, 0);
            }
        }
    }

    uint8_t AllocateTextureDescriptorSet(VkDevice device, ROResources& readOnlies, DescriptorContext& descriptorContext)
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
        BlitCL::DynamicArray<VkDescriptorImageInfo> imageInfos{ BlitzenCore::Ce_MaxTextureCount };
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
}