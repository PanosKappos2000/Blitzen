#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
    void CreateBuffer(VmaAllocator allocator, AllocatedBuffer& buffer, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, VkDeviceSize bufferSize, 
    VmaAllocationCreateFlags allocationFlags)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.flags = 0;
        bufferInfo.pNext = nullptr;
        bufferInfo.usage = bufferUsage;
        bufferInfo.size = bufferSize;

        VmaAllocationCreateInfo bufferAllocationInfo{};
        bufferAllocationInfo.usage = memoryUsage;
        bufferAllocationInfo.flags = allocationFlags;

        VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &bufferAllocationInfo, &(buffer.buffer), &(buffer.allocation), &(buffer.allocationInfo)));
    }

    VkDeviceAddress GetBufferAddress(VkDevice device, VkBuffer buffer)
    {
        VkBufferDeviceAddressInfo indirectBufferAddressInfo{};
        indirectBufferAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        indirectBufferAddressInfo.pNext = nullptr;// For now I'm assuming that no extension will be needed here (I think there are none in the spec anyway)
        indirectBufferAddressInfo.buffer = buffer;
        return vkGetBufferDeviceAddress(device, &indirectBufferAddressInfo);
    }

    void CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags imageUsage, 
    uint8_t mipLevels /*= 1*/)
    {
        image.extent = extent;
        image.format = format;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = 0;
        imageInfo.pNext = nullptr;
        imageInfo.extent = extent;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.mipLevels  = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = imageUsage;
        
        VmaAllocationCreateInfo imageAllocationInfo{};
        imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(allocator, &imageInfo, &imageAllocationInfo, &(image.image), &(image.allocation), nullptr))

        CreateImageView(device, image.imageView, image.image, format, mipLevels);
    }

    void CreateImageView(VkDevice device, VkImageView& imageView, VkImage image, VkFormat format, uint8_t mipLevels)
    {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        info.image = image;
        info.format = format;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.subresourceRange.aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? 
        VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = mipLevels;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VK_CHECK(vkCreateImageView(device, &info, nullptr, &imageView))
    }

    void CreateTextureImage(void* data, VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, 
    VkCommandBuffer commandBuffer, VkQueue queue, uint8_t loadMipMaps /*= 0*/)
    {
        VkDeviceSize imageSize = extent.width * extent.height * extent.depth * 4;
        AllocatedBuffer stagingBuffer;
        CreateBuffer(allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, imageSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);

        BlitzenCore::BlitMemCopy(stagingBuffer.allocationInfo.pMappedData, data, imageSize);

        CreateImage(device, allocator, image, extent, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VkImageMemoryBarrier2 imageMemoryBarrier{};
        ImageMemoryBarrier(image.image, imageMemoryBarrier, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 
        VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        CopyBufferToImage(commandBuffer, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, extent);

        VkImageMemoryBarrier2 secondTransitionBarrier{};
        ImageMemoryBarrier(image.image, secondTransitionBarrier, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 
        VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, 
        VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &secondTransitionBarrier);

        SubmitCommandBuffer(queue, commandBuffer);
        vkQueueWaitIdle(queue);

        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
    }

    void CreateTextureSampler(VkDevice device, VkSampler& sampler)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.flags = 0;
        samplerInfo.pNext = nullptr;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 4.f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.mipLodBias = 0.f;
        samplerInfo.maxLod = 0.f;
        samplerInfo.minLod = 0.f;

        VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &sampler))
    }

    VkSampler CreateSampler(VkDevice device)
    {
        VkSamplerCreateInfo createInfo {}; 
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	    createInfo.magFilter = VK_FILTER_NEAREST;
	    createInfo.minFilter = VK_FILTER_NEAREST;
	    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	    createInfo.minLod = 0;
	    createInfo.maxLod = 16.f;

	    VkSampler sampler = VK_NULL_HANDLE;
	    VK_CHECK(vkCreateSampler(device, &createInfo, 0, &sampler))
	    return sampler;
    }

    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, 
    VkExtent2D srcImageSize, VkExtent2D dstImageSize, VkImageSubresourceLayers& srcImageSL, VkImageSubresourceLayers& dstImageSL)
    {
        /*
            This function is pretty hardcoded for now and for a specific use. I don't think I will be needing that many image copies, but we'll see
        */

        VkImageBlit2 imageRegion{};
        imageRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
        imageRegion.pNext = nullptr;
        imageRegion.srcOffsets[1].x = static_cast<int32_t>(srcImageSize.width);
        imageRegion.srcOffsets[1].y = static_cast<int32_t>(srcImageSize.height);
        imageRegion.srcOffsets[1].z = 1;
        imageRegion.srcSubresource = srcImageSL;
        imageRegion.dstOffsets[1].x = dstImageSize.width;
        imageRegion.dstOffsets[1].y = dstImageSize.height;
        imageRegion.dstOffsets[1].z = 1;
        imageRegion.dstSubresource = dstImageSL;

        VkBlitImageInfo2 blitImage{};
        blitImage.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
        blitImage.pNext = nullptr;
        blitImage.srcImage = srcImage; 
        blitImage.srcImageLayout = srcLayout;
        blitImage.dstImage = dstImage;
        blitImage.dstImageLayout = dstLayout;
        blitImage.regionCount = 1;
        blitImage.pRegions = &imageRegion;
        vkCmdBlitImage2(commandBuffer, &blitImage);
    }

    void CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize copySize, VkDeviceSize srcOffset, 
    VkDeviceSize dstOffset)
    {
        VkBufferCopy2 copyRegion{};
        copyRegion.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
        copyRegion.size = copySize;
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;

        VkCopyBufferInfo2 copyInfo{};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
        copyInfo.pNext = nullptr;
        copyInfo.srcBuffer = srcBuffer;
        copyInfo.dstBuffer = dstBuffer;
        copyInfo.regionCount = 1; // Hardcoded, I don't see why I would ever need 2 copy regions
        copyInfo.pRegions = &copyRegion;

        vkCmdCopyBuffer2(commandBuffer, &copyInfo);
    }

    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage image, VkImageLayout imageLayout, VkExtent3D extent)
    {
        VkBufferImageCopy2 copyRegion{};
        copyRegion.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
        copyRegion.pNext = nullptr;
        copyRegion.imageExtent = extent;
        // most things here are pretty hardcoded, but I don't know if I'll actually needs this again anytime soon
        copyRegion.imageOffset = { 0, 0, 0 };
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.bufferOffset = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.bufferRowLength = 0;

        VkCopyBufferToImageInfo2 copyInfo{};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
        copyInfo.pNext = nullptr;
        copyInfo.srcBuffer = srcBuffer;
        copyInfo.dstImage = image;
        copyInfo.dstImageLayout = imageLayout;
        copyInfo.regionCount = 1;
        copyInfo.pRegions = &copyRegion;
        vkCmdCopyBufferToImage2(commandBuffer, &copyInfo);
    }

    void AllocatedImage::CleanupResources(VmaAllocator allocator, VkDevice device)
    {
        vmaDestroyImage(allocator, image, allocation);
        vkDestroyImageView(device, imageView, nullptr);
    }

    void AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout* pLayouts, uint32_t descriptorSetCount, VkDescriptorSet* pSets)
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = pool;
        allocInfo.pSetLayouts = pLayouts;
        allocInfo.descriptorSetCount = descriptorSetCount;
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, pSets));
    }

    void WriteBufferDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorBufferInfo& bufferInfo, VkDescriptorType descriptorType, VkDescriptorSet dstSet, 
    uint32_t dstBinding, uint32_t descriptorCount, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
    {
        bufferInfo.buffer = buffer;
        bufferInfo.offset = offset;
        bufferInfo.range = range;

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.descriptorType = descriptorType;
        write.dstSet = dstSet;
        write.dstBinding = dstBinding;
        write.descriptorCount = descriptorCount;
        write.pBufferInfo = &bufferInfo;
    }

    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo* pImageInfos, VkDescriptorType descriptorType, VkDescriptorSet dstSet, 
    uint32_t descriptorCount, uint32_t binding)
    {
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = dstSet;
        write.dstBinding = binding;
        write.descriptorType = descriptorType;
        write.descriptorCount = descriptorCount;
        write.pImageInfo = pImageInfos;
    }

    void PipelineBarrier(VkCommandBuffer commandBuffer, uint32_t memoryBarrierCount, VkMemoryBarrier2* pMemoryBarriers, uint32_t bufferBarrierCount, 
    VkBufferMemoryBarrier2* pBufferBarriers, uint32_t imageBarrierCount, VkImageMemoryBarrier2* pImageBarriers)
    {
        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.pNext = nullptr;
        dependency.bufferMemoryBarrierCount = bufferBarrierCount;
        dependency.pBufferMemoryBarriers = pBufferBarriers;
        dependency.memoryBarrierCount = memoryBarrierCount;
        dependency.pMemoryBarriers = pMemoryBarriers;
        dependency.imageMemoryBarrierCount = imageBarrierCount;
        dependency.pImageMemoryBarriers = pImageBarriers;
        vkCmdPipelineBarrier2(commandBuffer, &dependency);
    }

    void ImageMemoryBarrier(VkImage image, VkImageMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, 
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkImageLayout oldLayout, VkImageLayout newLayout, 
    VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer /* = 0 */, 
    uint32_t layerCount /* = VK_REMAINING_ARRAY_LAYERS */)
    {
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = image;
        barrier.srcStageMask = firstSyncStage;
        barrier.srcAccessMask = firstAccessStage;
        barrier.dstStageMask = secondSyncStage;
        barrier.dstAccessMask = secondAccessStage;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.baseMipLevel = baseMipLevel;
        barrier.subresourceRange.levelCount = levelCount;
        barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
        barrier.subresourceRange.layerCount = layerCount;
    }

    void BufferMemoryBarrier(VkBuffer buffer, VkBufferMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, 
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkDeviceSize offset, VkDeviceSize size)
    {
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        barrier.pNext = nullptr;
        barrier.buffer = buffer;
        barrier.srcStageMask = firstSyncStage;
        barrier.srcAccessMask = firstAccessStage;
        barrier.dstStageMask = secondSyncStage;
        barrier.dstAccessMask = secondAccessStage;
        barrier.offset = offset;
        barrier.size = size;
    }

    void MemoryBarrier(VkMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, 
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage)
    {
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        barrier.pNext = nullptr;
        barrier.srcStageMask = firstSyncStage;
        barrier.srcAccessMask = firstAccessStage;
        barrier.dstStageMask = secondSyncStage;
        barrier.dstAccessMask = secondAccessStage;
    }
}