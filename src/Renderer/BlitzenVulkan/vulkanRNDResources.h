#pragma once

#include "vulkanContext.h"

namespace BlitzenVulkan
{
    // Creates color attachment, depth attachment and depth pyramid for occlusion culling
    uint8_t RenderingAttachmentsInit(VkDevice device, VmaAllocator vma, ROResources& readOnlies, RWResources& readWrites, DescriptorContext& descriptorContext, PipelineContext& pipelineContext,
        uint32_t drawWidht, uint32_t drawHeight, uint32_t frame);

    // Creates the depth pyramid image and mip levels and their data. Needed for occlusion culling
    uint8_t CreateHI_Z(VkDevice device, VmaAllocator vma, DescriptorContext& descriptorContext, HI_Z_MAP& hiz, uint32_t drawWidth, uint32_t drawHeight, uint32_t frame, VkSampler sampler);

    template<class DATA>
    VkDeviceSize CreateSSBO(VmaAllocator allocator, VkDevice device, DATA* pData, BlitVk_SSBO& ssbo, Buffer& stagingBuffer, VkBufferUsageFlags usage, size_t elementCount)
    {
        if (elementCount == 0)
        {
            BLIT_ERROR("Cannot create buffer with size 0");
            return 0;
        }

        VkDeviceSize bufferSize{ elementCount * sizeof(DATA) };

        // SSBO
        if (!CreateBuffer(allocator, ssbo.m_buffer, usage, VMA_MEMORY_USAGE_GPU_ONLY, bufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            return 0;
        }

        // Staging buffer
        if (!CreateBuffer(allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, bufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            return 0;
        }

        void* pMapped = stagingBuffer.m_vmaInfo.pMappedData;
        if (!pMapped)
        {
            BLIT_ERROR("Failed to map pointer to staging buffer");
            return 0;
        }

        BlitzenCore::BlitMemCopy(pMapped, pData, bufferSize);

        // Success
        return bufferSize;
    }

    template<class DATA>
    VkDeviceSize CreateCPU_DATA_SSBO(VmaAllocator vma, VkDevice device, DATA* pData, BlitVk_CPU_DATA_SSBO<DATA>& ssbo, VkBufferUsageFlags usage, Buffer& tempStaging,
        size_t tempStagingElementCount, size_t persistentStagingElementCount, size_t tempStagingOffset)
    {
        if (persistentStagingElementCount + tempStagingOffset == 0)
        {
            BLIT_ERROR("Passed element count 0 to SSBO creation");
            return 0;
        }

        VkDeviceSize bufferSize{ sizeof(DATA) * (tempStagingOffset + tempStagingElementCount) };

        if (!CreateBuffer(vma, ssbo.m_buffer, usage, VMA_MEMORY_USAGE_GPU_ONLY, bufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("Failed to create SSBO resource");
            return 0;
        }

        size_t tempStagingSize{ sizeof(DATA) * tempStagingElementCount };

        if (tempStagingElementCount == 0)
        {
            BLIT_ERROR("Passed element count zero for temp staging buffer");
            return 0;
        }

        if (!CreateBuffer(vma, tempStaging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, tempStagingSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("Failed to create initial staging buffer");
            return 0;
        }

        DATA* pMappedTemp{ reinterpret_cast<DATA*>(tempStaging.m_vmaInfo.pMappedData) };
        if (!pMappedTemp)
        {
            BLIT_ERROR("Failed to map pointer to temporary staging buffer");
            return 0;
        }

        // PERSISTENT CPU DATA
        if (persistentStagingElementCount == 0)
        {
            BLIT_ERROR("Passed size 0 for persistent staging buffer, creating placeholder");
            persistentStagingElementCount = 1;
        }

        size_t persistentStagingSize{ sizeof(DATA) * persistentStagingElementCount };

        if (!CreateBuffer(vma, ssbo.m_staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, persistentStagingSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("Failed to Create peristent staging buffer for CPU WRITE SSBO");
            return 0;
        }

        ssbo.m_pMapped = reinterpret_cast<DATA*>(ssbo.m_staging.m_vmaInfo.pMappedData);
        if (!ssbo.m_pMapped)
        {
            BLIT_ERROR("Failed to map pointer to persistent staging buffer");
            return 0;
        }

        ssbo.m_copyDataSize = persistentStagingSize;
        BlitzenCore::BlitMemCopy(ssbo.m_pMapped, pData, ssbo.m_copyDataSize);
        BlitzenCore::BlitMemCopy(pMappedTemp, pData + tempStagingOffset, tempStagingSize);

        // Success
        return bufferSize;
    }

    template<class DATA>
    uint8_t CreateUBUFFER(VmaAllocator vma, VkDevice device, BlitVk_UBUFFER<DATA>& ubuffer, VkBufferUsageFlags usage)
    {
        if (!CreateBuffer(vma, ubuffer.m_buffer, usage, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(DATA), VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("Failed to create uniform buffer");
            return 0;
        }

        ubuffer.m_pMapped = reinterpret_cast<DATA*>(ubuffer.m_buffer.m_vmaInfo.pMappedData);

        if (!ubuffer.m_pMapped)
        {
            BLIT_ERROR("Failed to map uniform buffer address");
            return 0;
        }

        return 1;
    }
}