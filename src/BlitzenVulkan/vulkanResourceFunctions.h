#pragma once
#include "vulkanData.h"
#include "Renderer/blitDDSTextures.h"

namespace BlitzenVulkan
{
    // Create a allocator from the VMA library
    uint8_t CreateVmaAllocator(VkDevice device, VkInstance instance, 
        VkPhysicalDevice physicalDevice, VmaAllocator& allocator, 
        VmaAllocatorCreateFlags flags
    );

    // Allocates a buffer using VMA
    uint8_t CreateBuffer(VmaAllocator allocator, AllocatedBuffer& buffer, VkBufferUsageFlags bufferUsage, 
    VmaMemoryUsage memoryUsage, VkDeviceSize bufferSize, VmaAllocationCreateFlags allocationFlags);

    // Create a gpu only storage buffer and a staging buffer to hold its data
    // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set, the pAddr pointer is updated as well
    VkDeviceAddress CreateStorageBufferWithStagingBuffer(
    VmaAllocator allocator, VkDevice device, 
    void* pData, AllocatedBuffer& storageBuffer, AllocatedBuffer& stagingBuffer, 
    VkBufferUsageFlags usage, VkDeviceSize size);

    // Returns the GPU address of a buffer
    VkDeviceAddress GetBufferAddress(VkDevice device, VkBuffer buffer);

    // Creates and image with VMA and also create an image view for it
    uint8_t CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, 
        VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, 
        uint8_t mipLevels = 1, VmaMemoryUsage memoryUsage =VMA_MEMORY_USAGE_GPU_ONLY);

    uint8_t CreateImageView(VkDevice device, VkImageView& imageView, VkImage image, 
        VkFormat format, uint8_t baseMipLevel, uint8_t mipLevels
    );

    // Allocate an image resource to be used specifically as texture. 
    // The 1st parameter should be the loaded image data that should be passed to the image resource
    // This function should not be used if possible, the one below is overall better
    void CreateTextureImage(void* data, VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, 
    VkFormat format, VkImageUsageFlags usage, VkCommandBuffer commandBuffer, VkQueue queue, uint8_t mipLevels = 1);

    // This function is similar to the above but it gives its own buffer and mip levels are required. 
    // The buffer should already hold the texture data in pMappedData
    uint8_t CreateTextureImage(AllocatedBuffer& buffer, VkDevice device, VmaAllocator allocator, AllocatedImage& image, 
    VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, VkCommandBuffer commandBuffer, VkQueue queue, uint8_t mipLevels);

    VkSampler CreateSampler(VkDevice device, VkFilter filter, VkSamplerMipmapMode mipmapMode, 
        VkSamplerAddressMode addressMode, void* pNextChain = nullptr
    );

    // Returns VkFormat based on DDS input to correctly load a texture image
    VkFormat GetDDSVulkanFormat(const BlitzenEngine::DDS_HEADER& header, const BlitzenEngine::DDS_HEADER_DXT10& header10);

    // Uses vkCmdBlitImage2 to copy a source image to a destination image. Hardcodes alot of parameters. 
    //Can be improved but this is used rarely for now, so I will leave it as is until I have to
    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, 
    VkImage dstImage, VkImageLayout dstLayout, VkExtent2D srcImageSize, VkExtent2D dstImageSize, 
    VkImageSubresourceLayers srcImageSL, VkImageSubresourceLayers dstImageSL, VkFilter filter);

    // Copies data held by a buffer to an image. Used in texture image creation to hold the texture data in the buffer and then pass it to the image
    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage image, VkImageLayout imageLayout, VkExtent3D extent);

    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout imageLayout, 
    uint32_t bufferImageCopyRegionCount, VkBufferImageCopy2* bufferImageCopyRegions);

    // Create a copy VkBufferImageCopy2 to be passed to the array that will be passed to CopyBufferToImage function above
    void CreateCopyBufferToImageRegion(VkBufferImageCopy2& result, VkExtent3D imageExtent, VkOffset3D imageOffset, 
    VkImageAspectFlags aspectMask, uint32_t mipLevel, uint32_t baseArrayLayer, uint32_t layerCount, VkDeviceSize bufferOffset, 
    uint32_t bufferImageHeight, uint32_t bufferRowLength);

    // Creates a descriptor pool for descriptor sets whose memory should be managed by one and are not push descriptors (managed by command buffer)
    VkDescriptorPool CreateDescriptorPool(VkDevice device, uint32_t poolSizeCount, VkDescriptorPoolSize* pPoolSizes, uint32_t maxSets);

    // Allocates one or more descriptor sets whose memory will be managed by a descriptor pool
    uint8_t AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout* pLayouts, 
    uint32_t descriptorSetCount, VkDescriptorSet* pSets);
}