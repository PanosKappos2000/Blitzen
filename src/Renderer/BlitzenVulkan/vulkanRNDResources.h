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

    struct VK_CPU_DATA_BUFFER_SIZE_INFO
    {
        uint32_t m_fullSSBOSize{ 0 };
        uint32_t m_dynamicDataSize{ 0 };
        uint32_t m_dynamicDataOffset{ 0 };
        uint32_t m_staticDataSize{ 0 };
        uint32_t m_staticDataOffset{ 0 };
    };

    template<class DATA>
    VkDeviceSize CreateCPU_DATA_SSBO(VmaAllocator vma, VkDevice device, DATA* pData, BlitVk_CPU_DATA_SSBO<DATA>& ssbo, VkBufferUsageFlags usage, Buffer& tempStaging,
        VK_CPU_DATA_BUFFER_SIZE_INFO& sizeInfo)
    {
        if (sizeInfo.m_fullSSBOSize == 0)
        {
            BLIT_ERROR("Tried to create CPU_DATA_SSBO with element count 0 for the full SSBO size");
            return 0;
        }

        if (sizeInfo.m_staticDataSize == 0)
        {
            BLIT_ERROR("Tried to create CPU_DATA_SSBO with element count 0 for the static data size. Some static data is expected for a CPU_DATA_SSBO to be created");
            return 0;
        }

        if (sizeInfo.m_dynamicDataSize == 0)
        {
            BLIT_ERROR("No dynamic data found for CPU_DATA_SSBO. Temporarily passing element count 1");
            sizeInfo.m_dynamicDataSize = 1;
        }

        VkDeviceSize bufferSize{ sizeof(DATA) * sizeInfo.m_fullSSBOSize };

        if (!CreateBuffer(vma, ssbo.m_buffer, usage, VMA_MEMORY_USAGE_GPU_ONLY, bufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("Failed to create SSBO resource");
            return 0;
        }

        size_t staticDataSize{ sizeof(DATA) * sizeInfo.m_staticDataSize };

        if (!CreateBuffer(vma, tempStaging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, staticDataSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
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

        size_t dynamicDataSize{ sizeof(DATA) * sizeInfo.m_dynamicDataSize };

        if (!CreateBuffer(vma, ssbo.m_staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, dynamicDataSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
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

        BlitzenCore::BlitMemCopy(ssbo.m_pMapped, pData + sizeInfo.m_dynamicDataOffset, dynamicDataSize);
        BlitzenCore::BlitMemCopy(pMappedTemp, pData + sizeInfo.m_staticDataOffset, staticDataSize);
        ssbo.m_copyDataSize = dynamicDataSize;

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

    // Call vkCmdPushDescriptorSetKHR extension function (This can be removed if I upgrade to Vulkan 1.4)
    inline void PushDescriptors(VkInstance instance, VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t set,
        uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites)
    {
        auto func = (PFN_vkCmdPushDescriptorSetKHR)vkGetInstanceProcAddr(instance, "vkCmdPushDescriptorSetKHR");
        if (func != nullptr)
        {
            func(commandBuffer, bindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
        }
    }
}