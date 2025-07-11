#pragma once
#include "Renderer/BlitzenVulkan/Context/vulkanContext.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"

namespace BlitzenVulkan
{
    // Creates color attachment, depth attachment and depth pyramid for occlusion culling
    uint8_t RenderingAttachmentsInit(VkDevice device, VmaAllocator vma, ROResources& readOnlies, RWResources& readWrites, DescriptorContext& descriptorContext, PipelineContext& pipelineContext,
        uint32_t drawWidht, uint32_t drawHeight, uint32_t frame);

    // Creates the depth pyramid image and mip levels and their data. Needed for occlusion culling
    uint8_t CreateHI_Z(VkDevice device, VmaAllocator vma, DescriptorContext& descriptorContext, HI_Z_MAP& hiz, uint32_t drawWidth, uint32_t drawHeight, uint32_t frame, VkSampler sampler);

    uint8_t CreateSwapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, uint32_t windowWidth, uint32_t windowHeight,
        Queue graphicsQueue, Queue presentQueue, Queue computeQueue, VkAllocationCallbacks* pCustomAllocator, Swapchain& newSwapchain, VkSwapchainKHR oldSwapchain);

    template<class DATA>
    VkDeviceSize CreateSSBO(VmaAllocator allocator, VkDevice device, BlitVk_SSBO& ssbo, VkBufferUsageFlags usage, uint32_t elementCount)
    {
        if (elementCount == 0)
        {
            return 0;
        }

        VkDeviceSize bufferSize{ elementCount * sizeof(DATA) };

        // SSBO
        if (!CreateBuffer(allocator, ssbo.m_buffer, usage, VMA_MEMORY_USAGE_GPU_ONLY, bufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            return 0;
        }

        // Success
        return bufferSize;
    }

    template<class DATA>
    struct BUFFER_STAGING_CONTEXT
    {
        BlitVk_STAGING<DATA> staging{};
        DATA* pData{ nullptr };
        uint32_t elementCount{ 0 };
        uint32_t offset{ 0 };
    };
    template<class DATA>
    uint8_t CreateStaging(VmaAllocator allocator, VkDevice device, BUFFER_STAGING_CONTEXT<DATA>& context)
    {
        if (context.elementCount == 0)
        {
            return 0;
        }

        context.staging.m_dataSize = context.elementCount * sizeof(DATA);

        if (!CreateBuffer(allocator, context.staging.m_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, context.staging.m_dataSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            return 0;
        }

        context.staging.m_pMapped = reinterpret_cast<DATA*>(context.staging.m_buffer.m_vmaInfo.pMappedData);
        if (!context.staging.m_pMapped)
        {
            return 0;
        }

        if (context.pData)
        {
            BlitzenCore::BlitMemCopy(context.staging.m_pMapped, context.pData + context.offset, context.staging.m_dataSize);
        }

        return 1;
    }

    template<class DATA>
    uint8_t CreateEmptyStaging(VmaAllocator allocator, VkDevice device, BlitVk_STAGING<DATA>& staging, uint32_t maxElementCount)
    {
        if (maxElementCount == 0)
        {
            return 0;
        }

        staging.m_dataSize = maxElementCount * sizeof(DATA);

        if (!CreateBuffer(allocator, staging.m_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, staging.m_dataSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            return 0;
        }

        staging.m_pMapped = reinterpret_cast<DATA*>(staging.m_buffer.m_vmaInfo.pMappedData);
        if (!staging.m_pMapped)
        {
            return 0;
        }

        //success
        return 1;
    }
    

    template<class DATA>
    uint8_t CreateUBUFFER(VmaAllocator vma, VkDevice device, BlitVk_UBUFFER<DATA>& ubuffer, VkBufferUsageFlags usage)
    {
        if (!CreateBuffer(vma, ubuffer.m_buffer, usage, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(DATA), VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            return 0;
        }

        ubuffer.m_pMapped = reinterpret_cast<DATA*>(ubuffer.m_buffer.m_vmaInfo.pMappedData);

        if (!ubuffer.m_pMapped)
        {
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