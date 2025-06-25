#include "Renderer/BlitzenVulkan/Context/vulkanRenderer.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanRNDResources.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanPipelines.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenVulkan
{
    void VulkanRenderer::UpdateObjectTransform(uint32_t transformId, BlitzenEngine::MeshTransform* pTransform)
    {
        auto pData = m_readWrites[m_currentFrame].m_transformBuffer.m_staging.m_pMapped;
        BlitzenCore::BlitMemCopy(pData + transformId, pTransform, sizeof(BlitzenEngine::MeshTransform));
    }

    void VulkanRenderer::CopyTargetToSwapchain(VkCommandBuffer cmdb)
    {
        auto& readWrites{ m_readWrites[m_currentFrame] };
        auto& cmd{ m_commandsContext[m_currentFrame] };

        // Image barriers to transition the layout of the color attachment and the swapchain image
        VkImageMemoryBarrier2 presentBarriers[2]{};
        ImageMemoryBarrier(readWrites.m_colorTarget.m_image.m_image.m_handle, presentBarriers[0], VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        ImageMemoryBarrier(m_swapchain.m_images[m_swapchainIDX], presentBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, BLIT_ARRAY_SIZE(presentBarriers), presentBarriers);

        VkDescriptorImageInfo swapchainDescInfo{};
        swapchainDescInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        swapchainDescInfo.imageView = m_swapchain.m_views[m_swapchainIDX];

        VkWriteDescriptorSet swapchainImageWrite{};
        WriteImageDescriptorSets(swapchainImageWrite, &swapchainDescInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, 1, Ce_SwapchainDescriptorBinding);

        VkWriteDescriptorSet colorAttachmentCopyWrite[2]{ m_descriptorContext.m_colorTargetDescriptor[m_currentFrame], swapchainImageWrite };
        PushDescriptors(m_instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.m_presentLayout.handle, 0, 2, colorAttachmentCopyWrite);

        // Extent push constant
        BlitML::vec2 presentImageExtentPcVal{ float(readWrites.m_colorTarget.m_image.m_width), float(readWrites.m_colorTarget.m_image.m_height) };
        vkCmdPushConstants(cmdb, m_pipelines.m_presentLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlitML::vec2), &presentImageExtentPcVal);

        // Dispatches copy shader
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.m_presentPso.handle);
        vkCmdDispatch(cmdb, readWrites.m_colorTarget.m_image.m_width / 8 + 1, readWrites.m_colorTarget.m_image.m_height / 8 + 1, 1);

        // Layout transition barrier
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(m_swapchain.m_images[m_swapchainIDX], presentImageBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);
    }

    void CopyPyramidToSwapchain(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame,
        Swapchain& swapchain, uint32_t swapchainIDX, uint32_t drawWidth, uint32_t drawHeight, uint32_t pyramidMip)
    {
        // Image barriers to transition the layout of the color attachment and the swapchain image
        VkImageMemoryBarrier2 presentBarriers[2]{};
        ImageMemoryBarrier(readWrites.m_colorTarget.m_image.m_image.m_handle, presentBarriers[0], VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        ImageMemoryBarrier(swapchain.m_images[swapchainIDX], presentBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, BLIT_ARRAY_SIZE(presentBarriers), presentBarriers);

        // Swapchain image and attachment image descriptors
        VkWriteDescriptorSet swapchainImageWrite{};
        VkDescriptorImageInfo swapchainImageDescriptorInfo{};
        swapchainImageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        swapchainImageDescriptorInfo.imageView = swapchain.m_views[swapchainIDX];

        WriteImageDescriptorSets(swapchainImageWrite, &swapchainImageDescriptorInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, Ce_SwapchainDescriptorBinding);

        descriptorContext.m_HI_Z_descInfo[frame].imageView = readWrites.m_HI_Z_MAP.m_levels[pyramidMip];
        uint32_t levelWidth = BlitML::Max(1u, (readWrites.m_HI_Z_MAP.m_pyramid.m_width) >> pyramidMip);
        uint32_t levelHeight = BlitML::Max(1u, (readWrites.m_HI_Z_MAP.m_pyramid.m_height) >> pyramidMip);

        VkWriteDescriptorSet hizWrite{};

        VkDescriptorImageInfo HI_Z_info{};
        HI_Z_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        HI_Z_info.imageView = readWrites.m_HI_Z_MAP.m_levels[pyramidMip];
        HI_Z_info.sampler = readWrites.m_depthTarget.m_samp.m_handle;

        WriteImageDescriptorSets(hizWrite, &HI_Z_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE, Ce_ColorTargetDescriptorBinding);

        VkWriteDescriptorSet colorAttachmentCopyWrite[2] =
        {
            hizWrite, swapchainImageWrite
        };
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_presentLayout.handle, 0, 2, colorAttachmentCopyWrite);

        // Extent push constant
        BlitML::vec2 presentImageExtentPcVal
        {
            float(swapchain.m_extent.width), float(swapchain.m_extent.height)
        };
        vkCmdPushConstants(cmdb, pipelineContext.m_presentLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlitML::vec2), &presentImageExtentPcVal);

        // Dispatches copy shader
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_presentPso.handle);
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(swapchain.m_extent.width, 8), swapchain.m_extent.height / 8 + 1, 1);

        // Layout transition barrier
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(swapchain.m_images[swapchainIDX], presentImageBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);
    }

    void RecreateSwapchain(VkDevice device, VkInstance instance, Swapchain& swapchainData, VkSurfaceKHR surface, VkPhysicalDevice pdv, VmaAllocator vma,
        PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources* readWrites, DescriptorContext& descriptorContext, uint32_t windowWidth, uint32_t windowHeight, uint32_t frame, Queue graphicsQueue, Queue presentQueue, Queue computeQueue)
    {
        vkDeviceWaitIdle(device);

        for (uint32_t img = 0; img < swapchainData.m_imageCount; ++img)
        {
            vkDestroyImageView(device, swapchainData.m_views[img], nullptr);
        }
        swapchainData.m_imageCount = 0;

        // Creates new swapchain, after saving the old handle to destroy it
        auto oldSwapchain = swapchainData.m_handle;
        CreateSwapchain(device, surface, pdv, windowWidth, windowHeight, graphicsQueue, presentQueue, computeQueue, nullptr, swapchainData, oldSwapchain);

        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);

        for (uint32_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            auto& rws{ readWrites[frame] };

            // This function will recreate the color target. It will also automatically destroy the old target image and image view, if they are valid
            BLIT_ASSERT(Create2DImageResource(device, vma, rws.m_colorTarget.m_image, windowWidth, windowHeight, Ce_ColorTargetFormat, Ce_ColorTargetUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY));

            descriptorContext.m_colorTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorContext.m_colorTargetDescInfo[frame].imageView = rws.m_colorTarget.m_image.m_view.m_handle;
            descriptorContext.m_colorTargetDescInfo[frame].sampler = rws.m_colorTarget.m_samp.m_handle;

            WriteImageDescriptorSets(descriptorContext.m_colorTargetDescriptor[frame], &descriptorContext.m_colorTargetDescInfo[frame], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE,
                Ce_ColorTargetDescriptorBinding);

            // Color attachment rendering info
            CreateRenderingAttachmentInfo(pipelineContext.m_colorTargetInfo[frame], rws.m_colorTarget.m_image.m_view.m_handle, Ce_ColorTargetLayout,
                VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, ce_WindowClearColor);
            
            // This function will recreate the depth target. It will also automatically destroys the old target image and image view, if they are valid
            BLIT_ASSERT(Create2DImageResource(device, vma, rws.m_depthTarget.m_image, windowWidth, windowHeight, Ce_DepthTargetFormat, Ce_DepthTargetUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY));

            descriptorContext.m_depthTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorContext.m_depthTargetDescInfo[frame].imageView = rws.m_depthTarget.m_image.m_view.m_handle;
            descriptorContext.m_depthTargetDescInfo[frame].sampler = rws.m_depthTarget.m_samp.m_handle;

            WriteImageDescriptorSets(descriptorContext.m_HI_Z_descriptors[Ce_DepthTargetDescriptorID + frame * 2], &descriptorContext.m_depthTargetDescInfo[frame],
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE, Ce_DepthTargetDescriptorBinding);

            CreateRenderingAttachmentInfo(pipelineContext.m_depthTargetInfo[frame], rws.m_depthTarget.m_image.m_view.m_handle, Ce_DepthTargetLayout, VK_ATTACHMENT_LOAD_OP_LOAD,
                VK_ATTACHMENT_STORE_OP_STORE, { 0, 0, 0, 0 }, { 0, 0 });

            // No automatic destruction for depth pyramid mips, as they use a generalize function for createion
            for (uint32_t level = 0; level < rws.m_HI_Z_MAP.m_levelCount; ++level)
            {
                vkDestroyImageView(device, rws.m_HI_Z_MAP.m_levels[level], nullptr);
            }

            // This function will recreate the HI-Z map. It will also automatically destroy the old map image and image view, if they are valid
            BLIT_ASSERT(CreateHI_Z(device, vma, descriptorContext, rws.m_HI_Z_MAP, rws.m_colorTarget.m_image.m_width, rws.m_colorTarget.m_image.m_height, frame, rws.m_depthTarget.m_samp.m_handle));
        }
    }
}