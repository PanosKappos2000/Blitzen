#include "vulkanRenderer.h"
#include "vulkanResourceFunctions.h"

namespace BlitzenVulkan
{
    VkDeviceSize CreateGlobalIndexBuffer(VkDevice device, VmaAllocator allocator, uint8_t bRaytracingSupported, 
        AllocatedBuffer& stagingIndexBuffer, AllocatedBuffer& indexBuffer, uint32_t* pIndexData, size_t indicesCount)
    {
        // Raytracing support needs additional flags
        uint32_t geometryBuffersRaytracingFlags = bRaytracingSupported ?
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR 
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            : 0;
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * indicesCount;
        if(indexBufferSize == 0)
        {
            return 0;
        }
        // Creates index buffer. If the function fails, return 0 to let the caller know
        CreateStorageBufferWithStagingBuffer(allocator, device, 
            pIndexData, indexBuffer, stagingIndexBuffer, 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |  geometryBuffersRaytracingFlags, 
            indexBufferSize);
        if(indexBuffer.bufferHandle == VK_NULL_HANDLE)
        {
            return 0;
        }
        return indexBufferSize;
    }

    uint8_t AllocateTextureDescriptorSet(VkDevice device, uint32_t textureCount, TextureData* pTextures, 
        VkDescriptorPool& descriptorPool, VkDescriptorSetLayout* pLayout, VkDescriptorSet& descriptorSet)
    {
        if(textureCount == 0)
        {
            return 0;
        }
        // Creates descriptor pool to allocate combined image samplers
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = textureCount;
        descriptorPool = CreateDescriptorPool(device, 1, &poolSize, 1);
        if(descriptorPool == VK_NULL_HANDLE)
        {
            return 0;
        }
        // Allocates the descriptor sets
        if(!AllocateDescriptorSets(device, descriptorPool, pLayout, 1, &descriptorSet))
        {
            return 0;
        }
        
        // Creates image infos for every texture to be passed to the VkWriteDescriptorSet
        BlitCL::DynamicArray<VkDescriptorImageInfo> imageInfos(textureCount);
        for(size_t i = 0; i < imageInfos.GetSize(); ++i)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = pTextures[i].image.imageView;
            imageInfos[i].sampler = pTextures[i].sampler;
        }
        VkWriteDescriptorSet write{};
        WriteImageDescriptorSets(write, imageInfos.Data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            descriptorSet, static_cast<uint32_t>(imageInfos.GetSize()), 0);
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

        return 1;
    }
}