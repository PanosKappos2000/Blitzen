#pragma once
#include "Renderer/BlitzenVulkan/Context/vulkanContext.h"

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
            BLIT_ERROR("%s: Received element count 0 for SSBO creation", BLIT_VK_SYSTEM);
            return 0;
        }

        VkDeviceSize bufferSize{ elementCount * sizeof(DATA) };

        // SSBO
        if (!CreateBuffer(allocator, ssbo.m_buffer, usage, VMA_MEMORY_USAGE_GPU_ONLY, bufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("%s: Failed to create SSBO", BLIT_VK_SYSTEM);
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
    };
    template<class DATA>
    uint8_t CreateStaging(VmaAllocator allocator, VkDevice device, BUFFER_STAGING_CONTEXT<DATA>& context)
    {
        if (context.elementCount == 0)
        {
            BLIT_ERROR("%s: Received element count 0 for staging buffer creation", BLIT_VK_SYSTEM);
            return 0;
        }

        if (!context.pData)
        {
            BLIT_ERROR("%s: Received null data for staging buffer creation", BLIT_VK_SYSTEM);
            return 0;
        }

        context.staging.m_dataSize = context.elementCount * sizeof(DATA);

        if (!CreateBuffer(allocator, context.staging.m_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, context.staging.m_dataSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("%s: Failed to create staging buffer resource", BLIT_VK_SYSTEM);
            return 0;
        }

        context.staging.m_pMapped = reinterpret_cast<DATA*>(context.staging.m_buffer.m_vmaInfo.pMappedData);
        if (!context.staging.m_pMapped)
        {
            BLIT_ERROR("%s: Failed to map pointer to staging buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        BlitzenCore::BlitMemCopy(context.staging.m_pMapped, context.pData, context.staging.m_dataSize);

        return 1;
    }
    
    template<class DATA>
    struct CPU_DATA_BUFFER_STAGING_INFOS
    {
        BUFFER_STAGING_CONTEXT<DATA> m_stagingContext{};
    };

    template<class DATA>
    uint8_t Create_CPU_DATA_BUFFER_Stagings(VmaAllocator allocator, VkDevice device, CPU_DATA_BUFFER_STAGING_INFOS<DATA>* contexts)
    {
        constexpr uint32_t ContextCount = 2;
        constexpr uint32_t StaticContextID = 0;
        constexpr uint32_t DynamicContextID = 1;

        auto& staticContext{ contexts[StaticContextID] };
        auto& dynamicContext{ contexts[DynamicContextID] };

        if (staticContext.m_stagingContext.elementCount == 0)
        {
            BLIT_ERROR("%s: Received element count 0 for static staging buffer creation", BLIT_VK_SYSTEM);
            return 0;
        }

        if (dynamicContext.m_stagingContext.elementCount == 0)
        {
            BLIT_ERROR("%s: Received element count 0 for dynamic staging buffer creation", BLIT_VK_SYSTEM);
            return 0;
        }

        if (!staticContext.m_stagingContext.pData)
        {
            BLIT_ERROR("%s: Received null data for static staging buffer creation", BLIT_VK_SYSTEM);
            return 0;
        }

        if (!staticContext.m_stagingContext.pData)
        {
            BLIT_ERROR("%s: Received null data for static staging buffer creation", BLIT_VK_SYSTEM);
            return 0;
        }

        staticContext.m_stagingContext.staging.m_dataSize = staticContext.m_stagingContext.elementCount * sizeof(DATA);
        dynamicContext.m_stagingContext.staging.m_dataSize = dynamicContext.m_stagingContext.elementCount * sizeof(DATA);

        if (!CreateBuffer(allocator, staticContext.m_stagingContext.staging.m_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
            staticContext.m_stagingContext.staging.m_dataSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("%s: Failed to create static staging buffer resource", BLIT_VK_SYSTEM);
            return 0;
        }

        if (!CreateBuffer(allocator, dynamicContext.m_stagingContext.staging.m_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
            dynamicContext.m_stagingContext.staging.m_dataSize, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("%s: Failed to create dynamic staging buffer resource", BLIT_VK_SYSTEM);
            return 0;
        }

        staticContext.m_stagingContext.staging.m_pMapped = reinterpret_cast<DATA*>(staticContext.m_stagingContext.staging.m_buffer.m_vmaInfo.pMappedData);
        if (!staticContext.m_stagingContext.staging.m_pMapped)
        {
            BLIT_ERROR("%s: Failed to map pointer to static staging buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        dynamicContext.m_stagingContext.staging.m_pMapped = reinterpret_cast<DATA*>(dynamicContext.m_stagingContext.staging.m_buffer.m_vmaInfo.pMappedData);
        if (!dynamicContext.m_stagingContext.staging.m_pMapped)
        {
            BLIT_ERROR("%s: Failed to map pointer to dynamic staging buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        BlitzenCore::BlitMemCopy(staticContext.m_stagingContext.staging.m_pMapped, staticContext.m_stagingContext.pData, staticContext.m_stagingContext.staging.m_dataSize);
        BlitzenCore::BlitMemCopy(dynamicContext.m_stagingContext.staging.m_pMapped, dynamicContext.m_stagingContext.pData, dynamicContext.m_stagingContext.staging.m_dataSize);
    }

    template<class DATA>
    uint8_t CreateUBUFFER(VmaAllocator vma, VkDevice device, BlitVk_UBUFFER<DATA>& ubuffer, VkBufferUsageFlags usage)
    {
        if (!CreateBuffer(vma, ubuffer.m_buffer, usage, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(DATA), VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("%s: Failed to create uniform buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        ubuffer.m_pMapped = reinterpret_cast<DATA*>(ubuffer.m_buffer.m_vmaInfo.pMappedData);

        if (!ubuffer.m_pMapped)
        {
            BLIT_ERROR("%s: Failed to map uniform buffer address", BLIT_VK_SYSTEM);
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