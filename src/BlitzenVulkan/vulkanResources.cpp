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

    void CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags imageUsage, 
    uint8_t mimaps /*= 0*/)
    {
        image.extent = extent;
        image.format = format;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = 0;
        imageInfo.pNext = nullptr;
        imageInfo.extent = extent;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.mipLevels  = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = imageUsage;
        
        VmaAllocationCreateInfo imageAllocationInfo{};
        imageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(allocator, &imageInfo, &imageAllocationInfo, &(image.image), &(image.allocation), nullptr))

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.flags = 0;
        imageViewInfo.pNext = nullptr;
        imageViewInfo.image = image.image;
        imageViewInfo.format = format;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.subresourceRange.aspectMask = (imageUsage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ? 
        VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &image.imageView))
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
        imageRegion.srcOffsets[1].x = srcImageSize.width;
        imageRegion.srcOffsets[1].y = dstImageSize.height;
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

    void AllocatedImage::CleanupResources(VmaAllocator allocator, VkDevice device)
    {
        vmaDestroyImage(allocator, image, allocation);
        vkDestroyImageView(device, imageView, nullptr);
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
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageSubresourceRange& imageSR)
    {
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.image = image;
        barrier.srcStageMask = firstSyncStage;
        barrier.srcAccessMask = firstAccessStage;
        barrier.dstStageMask = secondSyncStage;
        barrier.dstAccessMask = secondAccessStage;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.subresourceRange = imageSR;
    }
}