#pragma once
#include "Renderer/BlitzenVulkan/Context/vulkanData.h"
#include "Renderer/Resources/Textures/blitTextures.h"

namespace BlitzenVulkan
{
    // Create a allocator from the VMA library
    uint8_t CreateVmaAllocator(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VmaAllocator& allocator, VmaAllocatorCreateFlags flags);

    // Allocates a buffer using VMA
    uint8_t CreateBuffer(VmaAllocator allocator, Buffer& buffer, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, VkDeviceSize bufferSize, VmaAllocationCreateFlags allocationFlags);
    
    // Returns the GPU address of a buffer
    VkDeviceAddress GetBufferAddress(VkDevice device, VkBuffer buffer);

    // Creates and image with VMA and also create an image view for it
    uint8_t Create2DImageResource(VkDevice device, VmaAllocator allocator, BlitVk_2DIMAGE& image, uint32_t width, uint32_t height, VkFormat format,
        VkImageUsageFlags imageUsage, uint8_t mipLevels, VmaMemoryUsage memoryUsage);

    uint8_t CreateImageView(VkDevice device, VkImageView& imageView, VkImage image, VkFormat format, uint8_t baseMipLevel, uint8_t mipLevels);
    
    uint8_t Create2DTexture(Buffer& buffer, VkDevice device, VmaAllocator allocator, BlitVk_2DIMAGE& tex, VkExtent2D extent, VkFormat format, VkImageUsageFlags usage,
        VkCommandBuffer commandBuffer, VkQueue queue, uint8_t mipLevels);

    VkSampler CreateSampler(VkDevice device, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, void* pNextChain = nullptr);

    // Returns VkFormat based on DDS input to correctly load a texture image
    VkFormat GetDDSVulkanFormat(const BlitzenEngine::DDS_HEADER& header, const BlitzenEngine::DDS_HEADER_DXT10& header10);

    // Uses vkCmdBlitImage2 to copy a source image to a destination image. Hardcodes alot of parameters. 
    //Can be improved but this is used rarely for now, so I will leave it as is until I have to
    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, VkExtent2D srcImageSize, VkExtent2D dstImageSize, 
        VkImageSubresourceLayers srcImageSL, VkImageSubresourceLayers dstImageSL, VkFilter filter);

    // Copies data held by a buffer to an image. Used in texture image creation to hold the texture data in the buffer and then pass it to the image
    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage image, VkImageLayout imageLayout, VkExtent3D extent);

    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout imageLayout, 
        uint32_t bufferImageCopyRegionCount, VkBufferImageCopy2* bufferImageCopyRegions);

    // Create a copy VkBufferImageCopy2 to be passed to the array that will be passed to CopyBufferToImage function above
    void CreateCopyBufferToImageRegion(VkBufferImageCopy2& result, VkExtent3D imageExtent, VkOffset3D imageOffset, 
        VkImageAspectFlags aspectMask, uint32_t mipLevel, uint32_t baseArrayLayer, uint32_t layerCount, 
        VkDeviceSize bufferOffset, uint32_t bufferImageHeight, uint32_t bufferRowLength);

    // Creates a descriptor pool for descriptor sets whose memory should be managed by one and are not push descriptors (managed by command buffer)
    VkDescriptorPool CreateDescriptorPool(VkDevice device, uint32_t poolSizeCount, VkDescriptorPoolSize* pPoolSizes, uint32_t maxSets, 
        VkDescriptorPoolCreateFlags flags = 0);

    // Allocates one or more descriptor sets whose memory will be managed by a descriptor pool
    uint8_t AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout* pLayouts, 
        uint32_t descriptorSetCount, VkDescriptorSet* pSets);

    // Creates VkWriteDescriptorSet for a buffer type descriptor set
    void WriteBufferDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorBufferInfo* pBufferInfo, VkDescriptorType descriptorType, uint32_t dstBinding, void* pNextChain,
        VkDescriptorSet dstSet, uint32_t descriptorCount, uint32_t dstArrayElement);

    void CreatePushDescriptorWrite(VkWriteDescriptorSet& write, VkDescriptorBufferInfo& bufferInfo, VkBuffer buffer, VkDescriptorType type, uint32_t binding);

    // Creates VkWriteDescirptorSet for an image type descirptor set. The image info struct(s) need to be initialized outside
    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo* pImageInfos, 
        VkDescriptorType descriptorType, VkDescriptorSet dstSet,
        uint32_t descriptorCount, uint32_t binding, uint32_t dstArrayElement = 0);

    // Creates VkDescriptorImageInfo and uses it to create a VkWriteDescriptorSet for images. DescriptorCount is set to 1 by default
    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo* pImageInfo, VkDescriptorType descirptorType, VkDescriptorSet dstSet, uint32_t binding);

    // Records a command for a pipeline barrier with the specified memory, buffer and image barriers
    void PipelineBarrier(VkCommandBuffer commandBuffer, uint32_t memoryBarrierCount, VkMemoryBarrier2* pMemoryBarriers, uint32_t bufferBarrierCount,
        VkBufferMemoryBarrier2* pBufferBarriers, uint32_t imageBarrierCount, VkImageMemoryBarrier2* pImageBarriers);

    // Sets up an image memory barrier to be passed to the above function
    void ImageMemoryBarrier(VkImage image, VkImageMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage,
        VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer = 0, uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS);

    // Sets up a buffer memory barrier to be passed to the PipelineBarrier function
    void BufferMemoryBarrier(VkBuffer buffer, VkBufferMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage,
        VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkDeviceSize offset, VkDeviceSize size);

    // Sets up a memory barrier to be passed to the PipelineBarrier function
    void MemoryBarrier(VkMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, VkPipelineStageFlags2 secondSyncStage,
        VkAccessFlags2 secondAccessStage);

        // Copies parts of one buffer to parts of another, depending on the offsets that are passed
    void CopyBufferToBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize copySize, VkDeviceSize srcOffset, VkDeviceSize dstOffset);
}