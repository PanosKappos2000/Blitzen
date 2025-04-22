#include "vulkanRenderer.h"
#include "vulkanResourceFunctions.h"

namespace BlitzenVulkan
{
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