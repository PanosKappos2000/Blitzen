#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
    uint8_t CreateVmaAllocator(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VmaAllocator& allocator, 
    VmaAllocatorCreateFlags flags)
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.flags = flags;

        VkResult res = vmaCreateAllocator(&allocatorInfo, &allocator);
        if(res != VK_SUCCESS)
            return 0;
        return 1;
    }

    uint8_t CreateBuffer(VmaAllocator allocator, AllocatedBuffer& buffer, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, VkDeviceSize bufferSize, 
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

        VkResult res = vmaCreateBuffer(allocator, &bufferInfo, &bufferAllocationInfo, 
        &(buffer.buffer), &(buffer.allocation), &(buffer.allocationInfo));
        if(res != VK_SUCCESS)
            return 0;
        return 1;
    }

    VkDeviceAddress CreateStorageBufferWithStagingBuffer(VmaAllocator allocator, VkDevice device, 
    void* pData, AllocatedBuffer& storageBuffer, AllocatedBuffer& stagingBuffer, 
    VkBufferUsageFlags usage, VkDeviceSize size, uint8_t getBufferDeviceAddress /*=0*/)
    {
        // The function needs to return a device address but it is relevant only if the user requested it
        // I don't know why I wrote it like this but it doesn't really matter
        VkDeviceAddress res = {};

        // Creates the storage buffer
        if(!CreateBuffer(allocator, storageBuffer, usage, VMA_MEMORY_USAGE_GPU_ONLY, 
        size, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            // The way this function lets the user know that it failed is by initializing the storage buffer to null
            storageBuffer.buffer = VK_NULL_HANDLE;
            return res;
        }

        // Creates the staging buffer and copy the data to it
        if(!CreateBuffer(allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
        size, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            storageBuffer.buffer = VK_NULL_HANDLE;
            return res;
        }

        // Gets the persistent mapped pointer to the staging buffer and copy the data to it
        void* pVertexBufferData = stagingBuffer.allocationInfo.pMappedData;
        BlitzenCore::BlitMemCopy(pVertexBufferData, pData, size);

        // If a buffer device address is requested, it is retrieved and returned
        if(getBufferDeviceAddress)
            res = GetBufferAddress(device, storageBuffer.buffer);
        return res;
    }

    VkDeviceAddress GetBufferAddress(VkDevice device, VkBuffer buffer)
    {
        VkBufferDeviceAddressInfo indirectBufferAddressInfo{};
        indirectBufferAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        indirectBufferAddressInfo.pNext = nullptr;// For now I'm assuming that no extension will be needed here (I think there are none in the spec anyway)
        indirectBufferAddressInfo.buffer = buffer;
        return vkGetBufferDeviceAddress(device, &indirectBufferAddressInfo);
    }

    uint8_t CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags imageUsage, 
    uint8_t mipLevels /*= 1*/, VmaMemoryUsage memoryUsage /*=VMA_MEMORY_USAGE_GPU_ONLY*/)
    {
        // Save the extent and format of the image in the AllocatedImage structure
        image.extent = extent;
        image.format = format;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = 0;// Hardcoded, no use case for this for now
        imageInfo.pNext = nullptr;

        imageInfo.extent = extent;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;// No MSAA
        // Pass the mip levels (important for textures and depth pyramid and probably other resources in the future)
        imageInfo.mipLevels  = mipLevels;
        // Array layers are not used with the current API structure
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        // I will probably need to create a parameter for this if I start using cube maps
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;// I can't see why I would ever use anything other than optimal
        imageInfo.usage = imageUsage;
        
        VmaAllocationCreateInfo imageAllocationInfo{};
        imageAllocationInfo.usage = memoryUsage;
        imageAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkResult res = vmaCreateImage(allocator, &imageInfo, &imageAllocationInfo, &(image.image), &(image.allocation), nullptr);
        if(res != VK_SUCCESS)
            return 0;

        // Creates an image view for the image by default
        if(!CreateImageView(device, image.imageView, image.image, format, 0, mipLevels))
            return 0;

        return 1;
    }

    uint8_t CreateImageView(VkDevice device, VkImageView& imageView, VkImage image, VkFormat format, uint8_t baseMipLevel, uint8_t mipLevels)
    {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.flags = 0;
        info.pNext = nullptr;
        info.image = image;
        info.format = format;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;

        // The aspect mask of the subresource is derived from the fromat of the image
        info.subresourceRange.aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? 
        VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = baseMipLevel;
        info.subresourceRange.levelCount = mipLevels;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        // Create the image view
        VkResult res = vkCreateImageView(device, &info, nullptr, &imageView);
        if(res != VK_SUCCESS)
            return 0;
        return 1;
    }

    void CreateTextureImage(void* data, VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, 
    VkCommandBuffer commandBuffer, VkQueue queue, uint8_t mipLevels /*=1*/)
    {
        // Create a buffer that will hold the data of the texture
        VkDeviceSize imageSize = extent.width * extent.height * extent.depth * 4;
        AllocatedBuffer stagingBuffer;
        CreateBuffer(allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
        imageSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        // Copy the data to the buffer
        BlitzenCore::BlitMemCopy(stagingBuffer.allocationInfo.pMappedData, data, imageSize);

        // Create a Vulkan image, that will have VK_IMAGE_USAGE_TRANSFER_BIT as well as the usage provided
        CreateImage(device, allocator, image, extent, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, mipLevels);

        // Start recording commands
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // Transition the layout of the image to optimal for data transfer
        VkImageMemoryBarrier2 imageMemoryBarrier{};
        ImageMemoryBarrier(image.image, imageMemoryBarrier, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 
        VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        // Copy the buffer to the image
        CopyBufferToImage(commandBuffer, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, extent);

        // Trasition the layout of the image to be used as a texture
        VkImageMemoryBarrier2 secondTransitionBarrier{};
        ImageMemoryBarrier(image.image, secondTransitionBarrier, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 
        VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, 
        VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &secondTransitionBarrier);

        // Submit the command and wait for the queue to finish
        SubmitCommandBuffer(queue, commandBuffer);
        vkQueueWaitIdle(queue);

        // Destroy the buffer, it is no longer needed
        vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
    }

    void CreateTextureImage(AllocatedBuffer& buffer, VkDevice device, VmaAllocator allocator, AllocatedImage& image, 
    VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, VkCommandBuffer commandBuffer, VkQueue queue, uint8_t mipLevels)
    {
        // Create an image for the texture data to be copied into. 
        // Adds the VK_IMAGE_USAGE_TRANSFER_DST_BIT, so that it can accept the data transfer from the buffer
        CreateImage(device, allocator, image, extent, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, mipLevels);

        // Get the initial offset for the first mip level to be copied
        uint32_t bufferOffset = 0;
        uint32_t mipWidth = extent.width;
        uint32_t mipHeight = extent.height;

        uint32_t blockSize = (format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK || format == VK_FORMAT_BC4_SNORM_BLOCK 
		|| format == VK_FORMAT_BC4_UNORM_BLOCK) ? 8 : 16;

        // Create an array that will hold the copy regions for all mip levels
        BlitCL::DynamicArray<VkBufferImageCopy2> copyRegions(mipLevels);

        // Create the copy regions
        for(uint8_t i = 0; i < mipLevels; ++i)
        {
            CreateCopyBufferToImageRegion(copyRegions[i], {mipWidth, mipHeight, 1}, {0, 0, 0}, VK_IMAGE_ASPECT_COLOR_BIT, 
            i, 0, 1, bufferOffset, 0, 0);

            bufferOffset += ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;
		    mipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
		    mipHeight = mipHeight > 1 ? mipHeight / 2 : 1;
        }

        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // Create an image barrier for transiton to transfer dst optimal layout
        VkImageMemoryBarrier2 transitionToTransferDSToptimal{};
        ImageMemoryBarrier(image.image, transitionToTransferDSToptimal, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, 
        VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &transitionToTransferDSToptimal);

        CopyBufferToImage(commandBuffer, buffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, copyRegions.Data());

        VkImageMemoryBarrier2 transitionImageToShaderReadOptimal{};
        ImageMemoryBarrier(image.image, transitionImageToShaderReadOptimal, VK_PIPELINE_STAGE_2_COPY_BIT, 
        VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &transitionImageToShaderReadOptimal);

        SubmitCommandBuffer(queue, commandBuffer);
        vkQueueWaitIdle(queue);
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
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.f;
        samplerInfo.maxLod = 16.f;
        samplerInfo.minLod = 0.f;

        VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &sampler))
    }

    VkFormat GetDDSVulkanFormat(const BlitzenEngine::DDS_HEADER& header, const BlitzenEngine::DDS_HEADER_DXT10& header10)
    {
        if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT1"))
	        return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	    if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT3"))
	        return VK_FORMAT_BC2_UNORM_BLOCK;
	    if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT5"))
	        return VK_FORMAT_BC3_UNORM_BLOCK;

        if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DX10"))
	    {
	    	switch (header10.dxgiFormat)
	    	{
	    	    case BlitzenEngine::DXGI_FORMAT_BC1_UNORM:
	    	    case BlitzenEngine::DXGI_FORMAT_BC1_UNORM_SRGB:
	    	    	return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;

	    	    case BlitzenEngine::DXGI_FORMAT_BC2_UNORM:
	    	    case BlitzenEngine::DXGI_FORMAT_BC2_UNORM_SRGB:
	    	    	return VK_FORMAT_BC2_UNORM_BLOCK;

	    	    case BlitzenEngine::DXGI_FORMAT_BC3_UNORM:
	    	    case BlitzenEngine::DXGI_FORMAT_BC3_UNORM_SRGB:
	    	    	return VK_FORMAT_BC3_UNORM_BLOCK;

	    	    case BlitzenEngine::DXGI_FORMAT_BC4_UNORM:
	    	    	return VK_FORMAT_BC4_UNORM_BLOCK;

	    	    case BlitzenEngine::DXGI_FORMAT_BC4_SNORM:
	    	    	return VK_FORMAT_BC4_SNORM_BLOCK;

	    	    case BlitzenEngine::DXGI_FORMAT_BC5_UNORM:
	    	    	return VK_FORMAT_BC5_UNORM_BLOCK;

	    	    case BlitzenEngine::DXGI_FORMAT_BC5_SNORM:
	    	    	return VK_FORMAT_BC5_SNORM_BLOCK;

	    	    case BlitzenEngine::DXGI_FORMAT_BC6H_UF16:
	    	    	return VK_FORMAT_BC6H_UFLOAT_BLOCK;

	    	    case BlitzenEngine::DXGI_FORMAT_BC6H_SF16:
	    	        return VK_FORMAT_BC6H_SFLOAT_BLOCK;

	    	    case BlitzenEngine::DXGI_FORMAT_BC7_UNORM:
	    	    case BlitzenEngine::DXGI_FORMAT_BC7_UNORM_SRGB:
	    	    	return VK_FORMAT_BC7_UNORM_BLOCK;
	    	}
	    }
        
	    return VK_FORMAT_UNDEFINED;
    }

    VkSampler CreateSampler(VkDevice device, VkSamplerReductionMode reductionMode)
    {
        VkSamplerCreateInfo createInfo {}; 
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	    createInfo.magFilter = VK_FILTER_LINEAR;
	    createInfo.minFilter = VK_FILTER_LINEAR;
	    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	    createInfo.minLod = 0;
	    createInfo.maxLod = 16.f;

        VkSamplerReductionModeCreateInfo reductionInfo{};
        reductionInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reductionInfo.reductionMode = reductionMode;
        createInfo.pNext = &reductionInfo;

	    VkSampler sampler = VK_NULL_HANDLE;
	    VK_CHECK(vkCreateSampler(device, &createInfo, 0, &sampler))
	    return sampler;
    }

    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, 
    VkExtent2D srcImageSize, VkExtent2D dstImageSize, VkImageSubresourceLayers srcImageSL, VkImageSubresourceLayers dstImageSL, VkFilter filter)
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
        blitImage.filter = filter;

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

    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout imageLayout, 
    uint32_t bufferImageCopyRegionCount, VkBufferImageCopy2* bufferImageCopyRegions)
    {
        VkCopyBufferToImageInfo2 copyInfo{};
        copyInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
        copyInfo.pNext = nullptr;
        copyInfo.srcBuffer = srcBuffer;
        copyInfo.dstImage = dstImage;
        copyInfo.dstImageLayout = imageLayout;
        copyInfo.regionCount = bufferImageCopyRegionCount;
        copyInfo.pRegions = bufferImageCopyRegions;

        vkCmdCopyBufferToImage2(commandBuffer, &copyInfo);
    }

    void CreateCopyBufferToImageRegion(VkBufferImageCopy2& result, VkExtent3D imageExtent, VkOffset3D imageOffset, 
    VkImageAspectFlags aspectMask, uint32_t mipLevel, uint32_t baseArrayLayer, uint32_t layerCount, VkDeviceSize bufferOffset, 
    uint32_t bufferImageHeight, uint32_t bufferRowLength)
    {
        result.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
        result.pNext = nullptr;
        result.imageExtent = imageExtent;
        result.imageOffset = imageOffset;
        result.imageSubresource.aspectMask = aspectMask;
        result.imageSubresource.mipLevel = mipLevel;
        result.imageSubresource.baseArrayLayer = baseArrayLayer;
        result.imageSubresource.layerCount = layerCount;
        result.bufferOffset = bufferOffset;
        result.bufferImageHeight = bufferImageHeight;
        result.bufferRowLength = bufferRowLength;
    }

    void AllocatedImage::CleanupResources(VmaAllocator allocator, VkDevice device)
    {
        vmaDestroyImage(allocator, image, allocation);
        vkDestroyImageView(device, imageView, nullptr);
    }

    VkDescriptorPool CreateDescriptorPool(VkDevice device, uint32_t poolSizeCount, VkDescriptorPoolSize* pPoolSizes, uint32_t maxSets)
    {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = 0;// Hardcoded, there is no use case for descriptor pool flags for now
        poolInfo.pNext = nullptr;
        poolInfo.maxSets = maxSets;
        poolInfo.poolSizeCount = poolSizeCount;
        poolInfo.pPoolSizes = pPoolSizes;

        // Creates the descriptor pool
        VkDescriptorPool pool;
        VkResult res = vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
        if(res != VK_SUCCESS)
            return VK_NULL_HANDLE;
        return pool;
    }

    uint8_t AllocateDescriptorSets(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout* pLayouts, 
    uint32_t descriptorSetCount, VkDescriptorSet* pSets)
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = pool;
        allocInfo.pSetLayouts = pLayouts;
        allocInfo.descriptorSetCount = descriptorSetCount;

        // Allocate the descriptor sets
        VkResult res = vkAllocateDescriptorSets(device, &allocInfo, pSets);
        if(res != VK_SUCCESS)
            return 0;
        return 1;
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
        //write.dstArrayElement = 
        write.pImageInfo = pImageInfos;
    }

    void WriteImageDescriptorSets(VkWriteDescriptorSet& write, VkDescriptorImageInfo& imageInfo, VkDescriptorType descirptorType, VkDescriptorSet dstSet, 
    uint32_t binding, VkImageLayout layout, VkImageView imageView, VkSampler sampler /*= VK_NULL_HANDLE*/)
    {
        imageInfo.imageLayout = layout;
        imageInfo.imageView = imageView;
        imageInfo.sampler = sampler;

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = dstSet;
        write.dstBinding = binding;
        write.descriptorType = descirptorType;
        write.descriptorCount = 1; // Hardcoded, for more than one descriptor, the above function should be used
        write.pImageInfo = &imageInfo;
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