#include "vulkanRNDResources.h"
#include "vulkanResourceFunctions.h"
#include "vulkanPipelines.h"

namespace BlitzenVulkan
{
    uint8_t RenderingAttachmentsInit(VkDevice device, VmaAllocator vma, ROResources& readOnlies, RWResources& readWrites, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, 
        uint32_t drawWidth, uint32_t drawHeight, uint32_t frame)
    {
        readWrites.m_colorTarget.m_samp.m_handle = CreateSampler(device, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, nullptr);
        if (readWrites.m_colorTarget.m_samp.m_handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create color attachment sampler");
            return 0;
        }

        if (!Create2DImageResource(device, vma, readWrites.m_colorTarget.m_image, drawWidth, drawHeight, Ce_ColorTargetFormat, Ce_ColorTargetUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY))
        {
            BLIT_ERROR("Failed to create color target image");
            return 0;
        }

        descriptorContext.m_colorTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorContext.m_colorTargetDescInfo[frame].imageView = readWrites.m_colorTarget.m_image.m_view.m_handle;
        descriptorContext.m_colorTargetDescInfo[frame].sampler = readWrites.m_colorTarget.m_samp.m_handle;

        WriteImageDescriptorSets(descriptorContext.m_colorTargetDescriptor[frame], &descriptorContext.m_colorTargetDescInfo[frame], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE,
            Ce_ColorTargetDescriptorBinding);

        // Color attachment rendering info
        CreateRenderingAttachmentInfo(pipelineContext.m_colorTargetInfo[frame], readWrites.m_colorTarget.m_image.m_view.m_handle, Ce_ColorTargetLayout,
            VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, ce_WindowClearColor);


        VkSamplerReductionModeCreateInfo reductionInfo{};
        reductionInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reductionInfo.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;

        readWrites.m_depthTarget.m_samp.m_handle = CreateSampler(device, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, &reductionInfo);
        if (readWrites.m_depthTarget.m_samp.m_handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create depth target sampler");
            return 0;
        }

        if (!Create2DImageResource(device, vma, readWrites.m_depthTarget.m_image, drawWidth, drawHeight, Ce_DepthTargetFormat, Ce_DepthTargetUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY))
        {
            BLIT_ERROR("Failed to create depth target image");// Depth attachment rendering info
            return 0;
        }

        descriptorContext.m_depthTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorContext.m_depthTargetDescInfo[frame].imageView = readWrites.m_depthTarget.m_image.m_view.m_handle;
        descriptorContext.m_depthTargetDescInfo[frame].sampler = readWrites.m_depthTarget.m_samp.m_handle;

        WriteImageDescriptorSets(descriptorContext.m_HI_Z_descriptors[Ce_DepthTargetDescriptorID + frame * 2], &descriptorContext.m_depthTargetDescInfo[frame], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_NULL_HANDLE, Ce_DepthTargetDescriptorBinding);

        CreateRenderingAttachmentInfo(pipelineContext.m_depthTargetInfo[frame], readWrites.m_depthTarget.m_image.m_view.m_handle, Ce_DepthTargetLayout, VK_ATTACHMENT_LOAD_OP_LOAD, 
            VK_ATTACHMENT_STORE_OP_STORE, { 0, 0, 0, 0 }, { 0, 0 });


        // Depth pyramid
        if (!CreateHI_Z(device, vma, descriptorContext, readWrites.m_HI_Z_MAP, drawWidth, drawHeight, frame, readWrites.m_depthTarget.m_samp.m_handle))
        {
            BLIT_ERROR("Failed to create the depth pyramid");
            return 0;
        }

        // Success
        return 1;
    }

    uint8_t CreateHI_Z(VkDevice device, VmaAllocator vma, DescriptorContext& descriptorContext, HI_Z_MAP& hiz, uint32_t drawWidth, uint32_t drawHeight, uint32_t frame, VkSampler sampler)
    {
        // Non Conservative for tests with dx12
        //depthPyramidExtent.width = BlitML::Max(1u, (drawExtent.width) >> 1);
        //depthPyramidExtent.height = BlitML::Max(1u, (drawExtent.height) >> 1);

        // Conservative starting extent
        hiz.m_pyramid.m_width = BlitML::PreviousPow2(drawWidth);
        hiz.m_pyramid.m_height = BlitML::PreviousPow2(drawHeight);
        hiz.m_levelCount = BlitML::GetDepthPyramidMipLevels(hiz.m_pyramid.m_width, hiz.m_pyramid.m_height);

        if (!Create2DImageResource(device, vma, hiz.m_pyramid, hiz.m_pyramid.m_width, hiz.m_pyramid.m_height, Ce_DepthPyramidFormat, Ce_DepthPyramidImageUsage, hiz.m_levelCount, VMA_MEMORY_USAGE_GPU_ONLY))
        {
            BLIT_ERROR("Failed to create depth pyramid image");
            return 0;
        }

        descriptorContext.m_HI_Z_descInfo[frame].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        descriptorContext.m_HI_Z_descInfo[frame].imageView = hiz.m_pyramid.m_view.m_handle;
        descriptorContext.m_HI_Z_descInfo[frame].sampler = sampler;

        WriteImageDescriptorSets(descriptorContext.m_HI_Z_descriptors[Ce_HI_Z_MAPDescriptorID + frame * 2], &descriptorContext.m_HI_Z_descInfo[frame], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE,
            Ce_HI_Z_DstImageBinding);

        WriteImageDescriptorSets(descriptorContext.m_HI_Z_cullDescriptor[frame], &descriptorContext.m_HI_Z_descInfo[frame], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE,
            Ce_HI_Z_CullBinding);

        // Levels
        for (uint8_t i = 0; i < hiz.m_levelCount; ++i)
        {
            if (!CreateImageView(device, hiz.m_levels[i], hiz.m_pyramid.m_image.m_handle, Ce_DepthPyramidFormat, i, 1))
            {
                BLIT_ERROR("Failed to create depth pyramid mips");
                return 0;
            }
        }

        return 1;
    }
}