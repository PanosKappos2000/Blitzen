#include "vulkanRenderer.h"
#include "vulkanCommands.h"
#include "vulkanPipelines.h"
#include "vulkanResourceFunctions.h"
#include "vulkanRNDResources.h"
#include "Core/Events/blitTimeManager.h"

// Not necessary since I have my own math library
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"

namespace BlitzenVulkan
{
    static void DrawMeshTasks(VkInstance instance, VkCommandBuffer commandBuffer, VkBuffer drawBuffer, VkDeviceSize drawOffset, VkBuffer countBuffer, VkDeviceSize countOffset, 
        uint32_t maxDrawCount, uint32_t stride)
    {
        auto func = (PFN_vkCmdDrawMeshTasksIndirectCountEXT)vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectCountEXT");
        if (func != nullptr)
        {
            func(commandBuffer, drawBuffer, drawOffset, countBuffer, countOffset, maxDrawCount, stride);
        }
    }

    // Call vkCmdPushDescriptorSetKHR extension function (This can be removed if I upgrade to Vulkan 1.4)
    static void PushDescriptors(VkInstance instance, VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t set,
        uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites)
    {
        auto func = (PFN_vkCmdPushDescriptorSetKHR)vkGetInstanceProcAddr(instance, "vkCmdPushDescriptorSetKHR");
        if (func != nullptr)
        {
            func(commandBuffer, bindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
        }
    }

    // Fallback to glm function
    static glm::vec4 glm_NormalizePlane(glm::vec4& plane)
    {
        return plane / glm::length(glm::vec3(plane));
    }

    static void UpdateBuffers(BlitzenEngine::DrawContext& context, VulkanRenderer::FrameTools& tools, RWResources& buffers, VkQueue queue)
    {
        BeginCommandBuffer(tools.transferCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        CopyBufferToBuffer(tools.transferCommandBuffer, buffers.m_transformBuffer.m_staging.m_handle, buffers.m_transformBuffer.m_buffer.m_handle, buffers.m_transformBuffer.m_copyDataSize, 0, 0);

        // VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT is used here because the signal comes from a transfer queue.
        // More specific shader stages (like VERTEX or COMPUTE) are invalid for transfer queues per Vulkan spec.
        // This ensures compatibility with graphics queue work that reads the transform buffer.
        // DO NOT WASTE TIME TRYING TO CHANGE THIS
        VkSemaphoreSubmitInfo bufferCopySemaphoreInfo{};
        CreateSemahoreSubmitInfo(bufferCopySemaphoreInfo, tools.buffersReadySemaphore.handle, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
        SubmitCommandBuffer(queue, tools.transferCommandBuffer, 0, nullptr, 1, &bufferCopySemaphoreInfo);
    }

    static void BeginRendering(VkCommandBuffer commandBuffer, VkExtent2D renderAreaExtent, VkOffset2D renderAreaOffset,
        uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* pColorAttachments, VkRenderingAttachmentInfo* pDepthAttachment,
        VkRenderingAttachmentInfo* pStencilAttachment, uint32_t viewMask = 0, uint32_t layerCount = 1)
    {
        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.flags = 0;
        renderingInfo.pNext = nullptr;

        renderingInfo.viewMask = viewMask;
        renderingInfo.layerCount = layerCount;

        renderingInfo.renderArea.offset = renderAreaOffset;
        renderingInfo.renderArea.extent = renderAreaExtent;

        renderingInfo.colorAttachmentCount = colorAttachmentCount;
        renderingInfo.pColorAttachments = pColorAttachments;
        renderingInfo.pDepthAttachment = pDepthAttachment;
        renderingInfo.pStencilAttachment = pStencilAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);
    }

    static void DefineViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent)
    {
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = float(extent.height); // Start from full height (flips y axis)
        viewport.width = float(extent.width);
        viewport.height = -(float(extent.height));// Move a negative amount of full height (flips y axis)
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent.width = extent.width;
        scissor.extent.height = extent.height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;

        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    // Prepares the first culling compute pass (Frustum culling, lod selection, only previously visible objects)
    static void DrawCullFirstPass(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies, 
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame)
    {
        // Count reset barrier
        VkBufferMemoryBarrier2 countResetBarrier{};
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, countResetBarrier,  VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Execute 
        PipelineBarrier(cmdb, 0, nullptr, 1, &countResetBarrier, 0, nullptr);

        // Reset
        vkCmdFillBuffer(cmdb, readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

        // Barrier waits for count reset, last frame draw commands read and visibility buffer write
        VkBufferMemoryBarrier2 cullBarriers[3]{};
        // Count reset barrier
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, cullBarriers[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Commands read barrier
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, cullBarriers[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Visibility buffer Barrier
        BufferMemoryBarrier(readWrites.m_drawVisBuffer.m_buffer.m_handle, cullBarriers[2], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,  VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullBarriers), cullBarriers, 0, nullptr);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_CullDescriptorCount, 
            &descriptorContext.m_pushDescriptorsCull[frame * Ce_CullDescriptorCount]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[frame * Ce_SharedDescriptorCount]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_DrawOcclusionDescriptorCount,
            &descriptorContext.m_pushDescriptorsDrawOcc[frame]);

        // Pipeline and descriptors
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullFirstPso.handle);
        DrawCullShaderPushConstant pushConstant{ descriptorContext.m_opaqueRenderAddr, drawContext.m_renders.m_renderCount};
        vkCmdPushConstants(cmdb, pipelineContext.m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DrawCullShaderPushConstant), &pushConstant);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawContext.m_renders.m_renderCount, 64), 1, 1);

        // Barrier blocks graphics command and count read
        VkBufferMemoryBarrier2 graphicsBarriers[2]{};
        // Count
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, graphicsBarriers[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Commands
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, graphicsBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, 
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers, 0, nullptr);
    }

    // Prepares the second culling compute pass (frustum culling, LOD selection, occlusion, visibility setting)
    // Creates commands for previously culled objects. Sets visibility for non-culled objects (already drawn after previous pass)
    static void DrawCullOcclusionPass(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame)
    {
        // Count reset barrier
        VkBufferMemoryBarrier2 resetBarrier{};
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, resetBarrier, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Execute 
        PipelineBarrier(cmdb, 0, nullptr, 1, &resetBarrier, 0, nullptr);

        // Reset
        vkCmdFillBuffer(cmdb, readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

        // Barrier waits for count reset, last frame draw commands read and visibility buffer read
        VkBufferMemoryBarrier2 cullBarriers[3]{};
        // Count reset barrier
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, cullBarriers[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Commands read barrier
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, cullBarriers[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Visibility buffer Barrier
        BufferMemoryBarrier(readWrites.m_drawVisBuffer.m_buffer.m_handle, cullBarriers[2], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        
        // Additional image memory barrier for depth pyramid
        VkImageMemoryBarrier2 HI_Z_barrier{};
        ImageMemoryBarrier(readWrites.m_HI_Z_MAP.m_pyramid.m_image.m_handle, HI_Z_barrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullBarriers), cullBarriers, 1, &HI_Z_barrier);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_CullDescriptorCount, 
            &descriptorContext.m_pushDescriptorsCull[frame * Ce_CullDescriptorCount]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[frame * Ce_SharedDescriptorCount]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_DrawOcclusionDescriptorCount,
            &descriptorContext.m_pushDescriptorsDrawOcc[frame * Ce_DrawOcclusionDescriptorCount]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, 1, 
            &descriptorContext.m_HI_Z_cullDescriptor[frame]);

        // Pipeline and push Constants
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLatePso.handle);
        DrawCullShaderPushConstant pushConstant{ descriptorContext.m_opaqueRenderAddr, drawContext.m_renders.m_renderCount };
        vkCmdPushConstants(cmdb, pipelineContext.m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DrawCullShaderPushConstant), &pushConstant);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawContext.m_renders.m_renderCount, 64), 1, 1);

        // Barrier blocks graphics command and count read
        VkBufferMemoryBarrier2 graphicsBarriers[2] = {};
        // Count
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, graphicsBarriers[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Commands
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, graphicsBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers, 0, nullptr);
    }

    static void DrawCullTrans(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame)
    {
        // Count reset barrier
        VkBufferMemoryBarrier2 resetBarrier{};
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, resetBarrier, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Execute 
        PipelineBarrier(cmdb, 0, nullptr, 1, &resetBarrier, 0, nullptr);

        // Reset
        vkCmdFillBuffer(cmdb, readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

        // Barrier waits for count reset, last frame draw commands read and visibility buffer write
        VkBufferMemoryBarrier2 dispatchBarriers[2]{};
        // Count reset barrier
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, dispatchBarriers[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Commands read barrier
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, dispatchBarriers[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);

        // Additional image memory barrier for depth pyramid
        VkImageMemoryBarrier2 HI_Z_barrier{};
        ImageMemoryBarrier(readWrites.m_HI_Z_MAP.m_pyramid.m_image.m_handle, HI_Z_barrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

        // execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(dispatchBarriers), dispatchBarriers, 1, &HI_Z_barrier);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_CullDescriptorCount, 
            &descriptorContext.m_pushDescriptorsCull[Ce_CullDescriptorCount * frame]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[Ce_SharedDescriptorCount * frame]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, 1, 
            &descriptorContext.m_HI_Z_cullDescriptor[frame]);

        // Pipeline and descriptors
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_transDrawCullPso.handle);
        DrawCullShaderPushConstant pushConstant{ descriptorContext.m_transRenderAddr, drawContext.m_renders.m_transparentRenderCount };
        vkCmdPushConstants(cmdb, pipelineContext.m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DrawCullShaderPushConstant), &pushConstant);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawContext.m_renders.m_transparentRenderCount, 64), 1, 1);

        // Barrier blocks graphics command and count read
        VkBufferMemoryBarrier2 waitForCullingShader[2]{};
        // Count
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, waitForCullingShader[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Commands
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, waitForCullingShader[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(waitForCullingShader), waitForCullingShader, 0, nullptr);
    }

    static void ClusterDispatch(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame)
    {
        // Barrier before count reset
        VkBufferMemoryBarrier2 clusterResetBarrier{};
        BufferMemoryBarrier(readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, clusterResetBarrier, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // execute
        PipelineBarrier(cmdb, 0, nullptr, 1, &clusterResetBarrier, 0, nullptr);

        // Reset
        vkCmdFillBuffer(cmdb, readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

        // Barrier for previous frame cluster count and cluster dispatch read
        VkBufferMemoryBarrier2 cullBarriers[2]{};
        // Cluster count
        BufferMemoryBarrier(readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, cullBarriers[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Cluster dispatch
        BufferMemoryBarrier(readWrites.m_clusterGroupDataBuffer.m_buffer.m_handle, cullBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullBarriers), cullBarriers, 0, nullptr);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, Ce_PushDescriptorSetID, Ce_CullDescriptorCount, 
            &descriptorContext.m_pushDescriptorsCull[Ce_CullDescriptorCount * frame]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[Ce_SharedDescriptorCount * frame]);

		// Pipeline and push constants
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullDispatchPso.handle);
        ClusterCullShaderPushConstant pushConstant
        {
            descriptorContext.m_opaqueRenderAddr, descriptorContext.m_clusterGroupAddr[frame],
            descriptorContext.m_clusterCounterAddr[frame], drawContext.m_renders.m_renderCount
        };
        vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ClusterCullShaderPushConstant), &pushConstant);
        // Dispatch
        vkCmdDispatch(cmdb, (drawContext.m_renders.m_renderCount / 64) + 1, 1, 1);

        // Cluster dispatch read barrier
        VkBufferMemoryBarrier2 clusterCullBarrier{};
        BufferMemoryBarrier(readWrites.m_clusterGroupDataBuffer.m_buffer.m_handle, clusterCullBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // execute
        PipelineBarrier(cmdb, 0, nullptr, 1, &clusterCullBarrier, 0, nullptr);

        // Cluster count copy barrier
        VkBufferMemoryBarrier2 countCopyBarrier{};
        BufferMemoryBarrier(readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, countCopyBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, 0, sizeof(uint32_t));
        PipelineBarrier(cmdb, 0, nullptr, 1, &countCopyBarrier, 0, nullptr);
        // Copy
        CopyBufferToBuffer(cmdb, readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, readWrites.m_clusterDispatchCounterCopy.m_buffer.m_handle, sizeof(uint32_t), 0, 0);
    }

    static void ClusterCullDispatchTrans(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame)
    {
        // Barrier before count reset
        VkBufferMemoryBarrier2 clusterResetBarrier{};
        BufferMemoryBarrier(readWrites.m_transClusterDispatchCounterBuffer.m_buffer.m_handle, clusterResetBarrier, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // execute
        PipelineBarrier(cmdb, 0, nullptr, 1, &clusterResetBarrier, 0, nullptr);

        // Reset
        vkCmdFillBuffer(cmdb, readWrites.m_transClusterDispatchCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

        // Barrier for previous frame cluster count and cluster dispatch read
        VkBufferMemoryBarrier2 cullBarriers[2]{};
        // Cluster count
        BufferMemoryBarrier(readWrites.m_transClusterDispatchCounterBuffer.m_buffer.m_handle, cullBarriers[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Cluster dispatch
        BufferMemoryBarrier(readWrites.m_transClusterGroupDataBuffer.m_buffer.m_handle, cullBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);

        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullBarriers), cullBarriers, 0, nullptr);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, Ce_PushDescriptorSetID, Ce_CullDescriptorCount,
            &descriptorContext.m_pushDescriptorsCull[Ce_CullDescriptorCount * frame]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[Ce_SharedDescriptorCount * frame]);

        // Pipeline and push constants
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullDispatchPso.handle);
        ClusterCullShaderPushConstant pushConstant
        {
            descriptorContext.m_transRenderAddr, descriptorContext.m_transClusterGroupAddr[frame],
            descriptorContext.m_transClusterCounterAddr[frame], drawContext.m_renders.m_transparentRenderCount
        };
        vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ClusterCullShaderPushConstant), &pushConstant);
        // Dispatch
        vkCmdDispatch(cmdb, (drawContext.m_renders.m_transparentRenderCount / 64) + 1, 1, 1);

        // Cluster dispatch read barrier
        VkBufferMemoryBarrier2 clusterCullBarrier{};
        BufferMemoryBarrier(readWrites.m_transClusterGroupDataBuffer.m_buffer.m_handle, clusterCullBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // execute
        PipelineBarrier(cmdb, 0, nullptr, 1, &clusterCullBarrier, 0, nullptr);

        // Cluster count copy barrier
        VkBufferMemoryBarrier2 countCopyBarrier{};
        BufferMemoryBarrier(readWrites.m_transClusterDispatchCounterBuffer.m_buffer.m_handle, countCopyBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, 0, sizeof(uint32_t));
        PipelineBarrier(cmdb, 0, nullptr, 1, &countCopyBarrier, 0, nullptr);
        // Copy
        CopyBufferToBuffer(cmdb, readWrites.m_clusterDispatchCounterBuffer.m_buffer.m_handle, readWrites.m_clusterDispatchCounterCopy.m_buffer.m_handle, sizeof(uint32_t), 0, 0);
    }

    static void ClusterCull(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame, uint32_t dispatchCount)
    {
        // Draw count reset barrier
        VkBufferMemoryBarrier2 drawCountResetBarrier{};
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, drawCountResetBarrier, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, sizeof(uint32_t));
        PipelineBarrier(cmdb, 0, nullptr, 1, &drawCountResetBarrier, 0, nullptr);

        vkCmdFillBuffer(cmdb, readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

        // Wait for draw count reset, previous frame command read and cluster dispatch write
        VkBufferMemoryBarrier2 cullingShaders[3] = {};
        // Draw count reset
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, cullingShaders[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Command read barrier
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, cullingShaders[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Cluster dispatch barrier
        BufferMemoryBarrier(readWrites.m_clusterGroupDataBuffer.m_buffer.m_handle, cullingShaders[2], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execution
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullingShaders), cullingShaders, 0, nullptr);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, Ce_PushDescriptorSetID, Ce_CullDescriptorCount, 
            &descriptorContext.m_pushDescriptorsCull[frame * Ce_CullDescriptorCount]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[frame * Ce_SharedDescriptorCount]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, Ce_PushDescriptorSetID, Ce_ClusterCullDescriptorCount,
            descriptorContext.m_pushDescriptorsClusterCull);

        // Pipeline and push constants
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullPso.handle);
        ClusterCullShaderPushConstant pushConstant
        { 
            descriptorContext.m_opaqueRenderAddr, descriptorContext.m_clusterGroupAddr[frame], 0, dispatchCount
        };
        vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ClusterCullShaderPushConstant), &pushConstant);
        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(dispatchCount, 64) , 1, 1);

        // Barriers stop graphics command read and count read
        VkBufferMemoryBarrier2 waitForCullingShader[2]{};
        // Count read
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, waitForCullingShader[0],VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Command read
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, waitForCullingShader[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
            0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(waitForCullingShader), waitForCullingShader, 0, nullptr);
    }

    static void ClusterCullTransparent(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame, uint32_t dispatchCount)
    {
        VkBufferMemoryBarrier2 drawCountResetBarrier{};
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, drawCountResetBarrier, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, sizeof(uint32_t));
        PipelineBarrier(cmdb, 0, nullptr, 1, &drawCountResetBarrier, 0, nullptr);

        vkCmdFillBuffer(cmdb, readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, sizeof(uint32_t), 0);

        // Wait for draw count reset, previous frame command read and cluster dispatch write
        VkBufferMemoryBarrier2 cullingShaders[3] {};
        // Draw count reset
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, cullingShaders[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Command read barrier
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, cullingShaders[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Cluster dispatch barrier
        BufferMemoryBarrier(readWrites.m_transClusterGroupDataBuffer.m_buffer.m_handle, cullingShaders[2], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execution
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullingShaders), cullingShaders, 0, nullptr);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, Ce_PushDescriptorSetID, Ce_CullDescriptorCount,
            &descriptorContext.m_pushDescriptorsCull[frame * Ce_CullDescriptorCount]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[frame * Ce_SharedDescriptorCount]);

        // Pipeline and push constants
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_clusterCullPso.handle);
        ClusterCullShaderPushConstant pushConstant
        {
            descriptorContext.m_transRenderAddr, descriptorContext.m_transClusterGroupAddr[frame], 0, dispatchCount
        };
        vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ClusterCullShaderPushConstant), &pushConstant);
        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(dispatchCount, 64), 1, 1);

        // Barriers stop graphics command read and count read
        VkBufferMemoryBarrier2 waitForCullingShader[2]{};
        // Count read
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, waitForCullingShader[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Command read
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, waitForCullingShader[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
            0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(waitForCullingShader), waitForCullingShader, 0, nullptr);
    }

    static void ClusterBatch(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext)
    {

    }

    static void OpaqueMeshShader()
    {
        // If I reintroduce mesh shaders, this will be on its own function
        /*if (m_stats.meshShaderSupport)
        {
            DrawMeshTasks(m_instance, commandBuffer,
                m_currentStaticBuffers.indirectTaskBuffer.buffer.bufferHandle,
                offsetof(IndirectTaskData, drawIndirectTasks),
                m_currentStaticBuffers.indirectCountBuffer.buffer.bufferHandle,
                0, drawCount, sizeof(IndirectTaskData));
        }*/
    }

    static void DrawOpaque(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame, 
        VkExtent2D drawExtent, uint8_t latePass)
    {
        // Render pass begin
        pipelineContext.m_colorTargetInfo[frame].loadOp = latePass ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        pipelineContext.m_depthTargetInfo[frame].loadOp = latePass ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        BeginRendering(cmdb, drawExtent, {0, 0}, 1, &pipelineContext.m_colorTargetInfo[frame], &pipelineContext.m_depthTargetInfo[frame], nullptr);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_PushDescriptorSetID, Ce_GraphicsDescriptorCount,
            descriptorContext.m_pushDescriptorsGraphics);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[frame * Ce_SharedDescriptorCount]);
        vkCmdBindDescriptorSets(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_TextureDescriptorsSetID, 1,
            &descriptorContext.m_textureDescriptorSet, 0, nullptr);

        // Push constants
        GlobalShaderDataPushConstant pcData{ descriptorContext.m_opaqueRenderAddr };
        vkCmdPushConstants(cmdb, pipelineContext.m_opaqueDrawLayout.handle, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GlobalShaderDataPushConstant), &pcData);

        // Draw
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawPso.handle);
        vkCmdBindIndexBuffer(cmdb, readOnlies.m_idxBuffer.m_buffer.m_handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirectCount(cmdb, readWrites.m_drawCmdBuffer.m_buffer.m_handle, offsetof(IndirectDrawData, drawIndirect),
            readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, Ce_DrawCmdElementCount, sizeof(IndirectDrawData));

        // End pass
        vkCmdEndRendering(cmdb);
    }

    static void DrawOpaqueRT()
    {
        // Raytracing
        //VkWriteDescriptorSetAccelerationStructureKHR layoutAccelerationStructurePNext{};
        //if (bRaytracing)
        //{
        //    layoutAccelerationStructurePNext.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        //    layoutAccelerationStructurePNext.accelerationStructureCount = tlasCount;
        //    layoutAccelerationStructurePNext.pAccelerationStructures = pTlas;
        //    //tlasBuffer.descriptorWrite.pNext = &layoutAccelerationStructurePNext;
        //    //PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, PushDescriptorSetID, 1, pDescriptorWrites);
        //}
    }

    static void DrawTransparents(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame, 
        VkExtent2D drawExtent)
    {
        // Render pass begin
        pipelineContext.m_colorTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        pipelineContext.m_colorTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        BeginRendering(cmdb, drawExtent, { 0, 0 }, 1, &pipelineContext.m_colorTargetInfo[frame], &pipelineContext.m_depthTargetInfo[frame], nullptr);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_PushDescriptorSetID, Ce_GraphicsDescriptorCount,
            descriptorContext.m_pushDescriptorsGraphics);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[frame * Ce_SharedDescriptorCount]);
        vkCmdBindDescriptorSets(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_TextureDescriptorsSetID, 1, 
            &descriptorContext.m_textureDescriptorSet, 0, nullptr);

        // Push constants
        GlobalShaderDataPushConstant pcData{ descriptorContext.m_transRenderAddr };
        vkCmdPushConstants(cmdb, pipelineContext.m_opaqueDrawLayout.handle, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GlobalShaderDataPushConstant), &pcData);

        // Draw
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_tranparentDrawPso.handle);
        vkCmdBindIndexBuffer(cmdb, readOnlies.m_idxBuffer.m_buffer.m_handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirectCount(cmdb, readWrites.m_drawCmdBuffer.m_buffer.m_handle, offsetof(IndirectDrawData, drawIndirect),
            readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, Ce_DrawCmdElementCount, sizeof(IndirectDrawData));

        // End pass
        vkCmdEndRendering(cmdb);
    }

    static void DrawTransRT()
    {

    }

    static void GenerateHiZ(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame)
    {
        VkImageMemoryBarrier2 HI_Z_barriers[2]{};
        // Depth attachment to shader read
        ImageMemoryBarrier(readWrites.m_depthTarget.m_image.m_image.m_handle, HI_Z_barriers[0], VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
            VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // Depth pyramid to shader write
        ImageMemoryBarrier(readWrites.m_HI_Z_MAP.m_pyramid.m_image.m_handle, HI_Z_barriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 2, HI_Z_barriers);

        // Creates the descriptor write array. Initially it will holds the depth attachment layout and image view
        descriptorContext.m_depthTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorContext.m_depthTargetDescInfo[frame].imageView = readWrites.m_depthTarget.m_image.m_view.m_handle;
        descriptorContext.m_depthTargetDescInfo[frame].sampler = readWrites.m_depthTarget.m_samp.m_handle;

        // Binds the compute pipeline. It will be dispatched for every loop iteration
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_hiZPso.handle);
        for(size_t i = 0; i < readWrites.m_HI_Z_MAP.m_levelCount; ++i)
        {
            // Updates image info for each iteration
            if(i != 0)
            {
                descriptorContext.m_depthTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                descriptorContext.m_depthTargetDescInfo[frame].imageView = readWrites.m_HI_Z_MAP.m_levels[i - 1];
            }

            descriptorContext.m_HI_Z_descInfo[frame].imageView = readWrites.m_HI_Z_MAP.m_levels[i];

            // Descriptors
            PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_hiZLayout.handle, 0, 2, &descriptorContext.m_HI_Z_descriptors[2 * frame]);

            // Mip size calculcations
            uint32_t levelWidth = BlitML::Max(1u, (readWrites.m_HI_Z_MAP.m_pyramid.m_width) >> i);
            uint32_t levelHeight = BlitML::Max(1u, (readWrites.m_HI_Z_MAP.m_pyramid.m_height) >> i);

            // Push constant for extent
            BlitML::vec2 pyramidLevelExtentPushConstant{float(levelWidth), float(levelHeight)};
            vkCmdPushConstants(cmdb, pipelineContext.m_hiZLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlitML::vec2), &pyramidLevelExtentPushConstant);

            // Dispatch the shader to generate the current mip level of the depth pyramid
            vkCmdDispatch(cmdb, levelWidth / 32 + 1, levelHeight / 32 + 1, 1);

            // Barrier for the next loop, since it will use the current mip as the read descriptor
            VkImageMemoryBarrier2 hizWriteBarrier{};
            ImageMemoryBarrier(readWrites.m_HI_Z_MAP.m_pyramid.m_image.m_handle, hizWriteBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, 
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 1, &hizWriteBarrier);
        }

        // Pipeline barrier to transition back to depth attachment optimal layout
        VkImageMemoryBarrier2 depthAttachmentReadBarrier{};
        ImageMemoryBarrier(readWrites.m_depthTarget.m_image.m_image.m_handle, depthAttachmentReadBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 1, &depthAttachmentReadBarrier);
    }

    static void CopyPyramidToSwapchain(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame, 
        Swapchain& swapchain, uint32_t swapchainIDX, uint32_t drawWidth, uint32_t drawHeight, uint32_t pyramidMip)
    {
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

    static void DrawBackgroundImage(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame)
    {
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_backgroundPso.handle);
        VkWriteDescriptorSet backgroundImageWrite{};
        VkDescriptorImageInfo backgroundImageInfo{};
        backgroundImageInfo.imageView = readWrites.m_colorTarget.m_image.m_view.m_handle;
        backgroundImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        WriteImageDescriptorSets(backgroundImageWrite, &backgroundImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, 0);

	    PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_backgroundLayout.handle, 0, 1, &backgroundImageWrite);

	    BackgroundShaderPushConstant pc;
	    pc.data1 = BlitML::vec4(1, 0, 0, 1);
	    pc.data2 = BlitML::vec4(0, 0, 1, 1);
	    vkCmdPushConstants(cmdb, pipelineContext.m_backgroundLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BackgroundShaderPushConstant), &pc);

	    vkCmdDispatch(cmdb, uint32_t(std::ceil(readWrites.m_colorTarget.m_image.m_width / 16.0)), uint32_t(std::ceil(readWrites.m_colorTarget.m_image.m_height / 16.0)), 1);
    }

    static void CopyToSwapchain(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame, 
        Swapchain& swapchain, uint32_t swapchainIDX)
    {

        VkDescriptorImageInfo swapchainDescInfo{};
        swapchainDescInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        swapchainDescInfo.imageView = swapchain.m_views[swapchainIDX];

        VkWriteDescriptorSet swapchainImageWrite{};
        WriteImageDescriptorSets(swapchainImageWrite, &swapchainDescInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, 1, Ce_SwapchainDescriptorBinding);

        VkWriteDescriptorSet colorAttachmentCopyWrite[2]{ descriptorContext.m_colorTargetDescriptor[frame], swapchainImageWrite};
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_presentLayout.handle, 0, 2, colorAttachmentCopyWrite);

        // Extent push constant
        BlitML::vec2 presentImageExtentPcVal{ float(readWrites.m_colorTarget.m_image.m_width), float(readWrites.m_colorTarget.m_image.m_height)};
        vkCmdPushConstants(cmdb, pipelineContext.m_presentLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlitML::vec2), &presentImageExtentPcVal);

        // Dispatches copy shader
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_presentPso.handle);
        vkCmdDispatch(cmdb, readWrites.m_colorTarget.m_image.m_width / 8 + 1, readWrites.m_colorTarget.m_image.m_height / 8 + 1, 1);

        // Layout transition barrier
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(swapchain.m_images[swapchainIDX], presentImageBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(cmdb, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);
    }

    static void Present(VkDevice device, VkQueue queue, VkSwapchainKHR* pSwapchains, uint32_t swapchainCount, uint32_t waitSemaphoreCount, VkSemaphore* pWaitSemaphores, 
        uint32_t* pImageIndices, VkResult* pResults = nullptr, void* pNextChain = nullptr)
    {
        VkPresentInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        info.pNext = pNextChain;

        info.swapchainCount = swapchainCount; // might never support this but who knows
        info.pSwapchains = pSwapchains;

        info.waitSemaphoreCount = waitSemaphoreCount;
        info.pWaitSemaphores = pWaitSemaphores;

        info.pImageIndices = pImageIndices;
        info.pResults = pResults;

        vkQueuePresentKHR(queue, &info);
    }

    static void RecreateSwapchain(VkDevice device, VkInstance instance, Swapchain& swapchainData, VkSurfaceKHR surface, VkPhysicalDevice pdv, VmaAllocator vma,
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
        
            // Destroys old color target
            vmaDestroyImage(vma, rws.m_colorTarget.m_image.m_image.m_handle, rws.m_colorTarget.m_image.m_image.m_vmaAlloc);
            vkDestroyImageView(device, rws.m_colorTarget.m_image.m_view.m_handle, nullptr);
        
            BLIT_ASSERT(Create2DImageResource(device, vma, rws.m_colorTarget.m_image, windowWidth, windowHeight, Ce_ColorTargetFormat, Ce_ColorTargetUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY));
            
            descriptorContext.m_colorTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorContext.m_colorTargetDescInfo[frame].imageView = rws.m_colorTarget.m_image.m_view.m_handle;
            descriptorContext.m_colorTargetDescInfo[frame].sampler = rws.m_colorTarget.m_samp.m_handle;
            
            WriteImageDescriptorSets(descriptorContext.m_colorTargetDescriptor[frame], &descriptorContext.m_colorTargetDescInfo[frame], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE,
                Ce_ColorTargetDescriptorBinding);
            
            // Color attachment rendering info
            CreateRenderingAttachmentInfo(pipelineContext.m_colorTargetInfo[frame], rws.m_colorTarget.m_image.m_view.m_handle, Ce_ColorTargetLayout,
                VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, ce_WindowClearColor);
            
            // Destroys old depth target
            vmaDestroyImage(vma, rws.m_depthTarget.m_image.m_image.m_handle, rws.m_depthTarget.m_image.m_image.m_vmaAlloc);
            vkDestroyImageView(device, rws.m_depthTarget.m_image.m_view.m_handle, nullptr);
            
            BLIT_ASSERT(Create2DImageResource(device, vma, rws.m_depthTarget.m_image, windowWidth, windowHeight, Ce_DepthTargetFormat, Ce_DepthTargetUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY));
            
            descriptorContext.m_depthTargetDescInfo[frame].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorContext.m_depthTargetDescInfo[frame].imageView = rws.m_depthTarget.m_image.m_view.m_handle;
            descriptorContext.m_depthTargetDescInfo[frame].sampler = rws.m_depthTarget.m_samp.m_handle;
            
            WriteImageDescriptorSets(descriptorContext.m_HI_Z_descriptors[Ce_DepthTargetDescriptorID + frame * 2], &descriptorContext.m_depthTargetDescInfo[frame], 
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE, Ce_DepthTargetDescriptorBinding);
            
            CreateRenderingAttachmentInfo(pipelineContext.m_depthTargetInfo[frame], rws.m_depthTarget.m_image.m_view.m_handle, Ce_DepthTargetLayout, VK_ATTACHMENT_LOAD_OP_LOAD,
                VK_ATTACHMENT_STORE_OP_STORE, { 0, 0, 0, 0 }, { 0, 0 });
            
            // Destroys old depth pyramid
            for (uint32_t level = 0; level < rws.m_HI_Z_MAP.m_levelCount; ++level)
            {
                vkDestroyImageView(device, rws.m_HI_Z_MAP.m_levels[level], nullptr);
            }
            vmaDestroyImage(vma, rws.m_HI_Z_MAP.m_pyramid.m_image.m_handle, rws.m_HI_Z_MAP.m_pyramid.m_image.m_vmaAlloc);
            vkDestroyImageView(device, rws.m_HI_Z_MAP.m_pyramid.m_view.m_handle, nullptr);
            
            BLIT_ASSERT(CreateHI_Z(device, vma, descriptorContext, rws.m_HI_Z_MAP, rws.m_colorTarget.m_image.m_width, rws.m_colorTarget.m_image.m_height, frame, rws.m_depthTarget.m_samp.m_handle));
        }
    }


    void VulkanRenderer::Update(const BlitzenEngine::DrawContext& context)
    {
        if (context.m_camera.transformData.bWindowResize)
        {
            m_drawWidth = (uint32_t)context.m_camera.transformData.windowWidth;
            m_drawHeight = (uint32_t)context.m_camera.transformData.windowHeight;

            RecreateSwapchain(m_device, m_instance, m_swapchainValues, m_surface.handle, m_physicalDevice, m_allocator, m_pipelines, m_readOnlies, m_readWrites, m_descriptorContext, 
                m_drawWidth, m_drawHeight, m_currentFrame, m_graphicsQueue, m_presentQueue, m_computeQueue);

            context.m_camera.viewData.pyramidWidth = float(m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_width);
            context.m_camera.viewData.pyramidHeight = float(m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_height);
        }
    }

    void VulkanRenderer::UpdateObjectTransform(uint32_t transformId, BlitzenEngine::MeshTransform* pTransform)
    {
        auto pData = m_readWrites[m_currentFrame].m_transformBuffer.m_pMapped;
        BlitzenCore::BlitMemCopy(pData + transformId, pTransform, sizeof(BlitzenEngine::MeshTransform));
    }

    void VulkanRenderer::DrawFrame(BlitzenEngine::DrawContext& context)
    {
        auto& fTools = m_frameToolsList[m_currentFrame];
        auto& readWrites = m_readWrites[m_currentFrame];

        // Waits for the fence in the current frame tools struct to be signaled and resets it for next time when it gets signalled
        vkWaitForFences(m_device, 1, &fTools.inFlightFence.handle, VK_TRUE, ce_fenceTimeout);
        VK_CHECK(vkResetFences(m_device, 1, &(fTools.inFlightFence.handle)));

        UpdateBuffers(context, fTools, readWrites, m_transferQueue.handle);
        if (context.m_camera.transformData.bFreezeFrustum)
        {
            // Only change the matrix that moves the camera if the freeze frustum debug functionality is active
            readWrites.m_viewDataBuffer.m_pMapped->projectionViewMatrix = context.m_camera.viewData.projectionViewMatrix;
        }
        else
        {
            *(readWrites.m_viewDataBuffer.m_pMapped) = context.m_camera.viewData;
        }

        // Swapchain image, needed to present the color attachment results
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_swapchainValues.m_handle, ce_swapchainImageTimeout, fTools.imageAcquiredSemaphore.handle, VK_NULL_HANDLE, &swapchainIdx);

        // Color attachment working layout depends on if there are any render objects
        auto colorAttachmentWorkingLayout = context.m_renders.m_renderCount ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

        if constexpr (BlitzenCore::Ce_BuildClusters)
        {
            // Fist culling pass with separate command buffer
            BeginCommandBuffer(fTools.computeCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			// Generates cluster dispatch data and count for the opaque render objects
            ClusterDispatch(fTools.computeCommandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            if (context.m_renders.m_transparentRenderCount != 0)
            {
                ClusterCullDispatchTrans(fTools.computeCommandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);
            }

            // Submits command buffer to generate cluster dispatch count
            VkSemaphoreSubmitInfo bufferUpdateWaitSemaphore{};
            CreateSemahoreSubmitInfo(bufferUpdateWaitSemaphore, fTools.buffersReadySemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            VkSemaphoreSubmitInfo waitForClusterData{};
            CreateSemahoreSubmitInfo(waitForClusterData, fTools.preClusterCullingDoneSemaphore.handle,VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            SubmitCommandBuffer(m_computeQueue.handle, fTools.computeCommandBuffer, 1, &bufferUpdateWaitSemaphore, 1, &waitForClusterData, fTools.preCulsterCullingFence.handle);
            vkWaitForFences(m_device, 1, &fTools.preCulsterCullingFence.handle, VK_TRUE, ce_fenceTimeout);
            vkResetFences(m_device, 1, &fTools.preCulsterCullingFence.handle);

            // Command recording begins again
            BeginCommandBuffer(fTools.commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            
            DefineViewportAndScissor(fTools.commandBuffer, m_swapchainValues.m_extent);

            // Attachment barriers for layout transitions before rendering
            VkImageMemoryBarrier2 renderPassBarriers[2] {};
            ImageMemoryBarrier(readWrites.m_colorTarget.m_image.m_image.m_handle, renderPassBarriers[0], VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, colorAttachmentWorkingLayout, VK_IMAGE_ASPECT_COLOR_BIT,
                0, VK_REMAINING_MIP_LEVELS);
            ImageMemoryBarrier(readWrites.m_depthTarget.m_image.m_image.m_handle, renderPassBarriers[1], VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 
                VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
            // execute
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, renderPassBarriers);

            auto dispatchCount{uint32_t( *reinterpret_cast<uint32_t*>(readWrites.m_clusterDispatchCounterCopy.m_buffer.m_vmaInfo.pMappedData)) };
            uint32_t transparentDispatchCount = 0;
            if (context.m_renders.m_transparentRenderCount)
            {
                transparentDispatchCount =  uint32_t(*reinterpret_cast<uint32_t*>(readWrites.m_transClusterDispatchCounterCopy.m_buffer.m_vmaInfo.pMappedData));
            }

            // Culls opaque render object clusters
            ClusterCull(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame, dispatchCount);

            DrawOpaque(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame, 
                {m_drawWidth, m_drawHeight}, 0);
            
            if (context.m_renders.m_transparentRenderCount != 0)
            {
                ClusterCullTransparent(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], 
                    m_descriptorContext, context, m_currentFrame, transparentDispatchCount);

                DrawTransparents(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame, 
                    {m_drawWidth, m_drawHeight});
            }


            // Image barriers to transition the layout of the color attachment and the swapchain image
            VkImageMemoryBarrier2 presentBarriers[2] = {};
            ImageMemoryBarrier(readWrites.m_colorTarget.m_image.m_image.m_handle, presentBarriers[0], VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, colorAttachmentWorkingLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            ImageMemoryBarrier(m_swapchainValues.m_images[size_t(swapchainIdx)], presentBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            // Execute
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, BLIT_ARRAY_SIZE(presentBarriers), presentBarriers);

            // Copies the color attachment to the swapchain image
            CopyToSwapchain(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame, m_swapchainValues, 
                swapchainIdx);

            VkSemaphoreSubmitInfo waitSemaphores[2]{ {}, {} };
            CreateSemahoreSubmitInfo(waitSemaphores[0], fTools.imageAcquiredSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            CreateSemahoreSubmitInfo(waitSemaphores[1], fTools.preClusterCullingDoneSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            VkSemaphoreSubmitInfo signalSemaphore{};
            CreateSemahoreSubmitInfo(signalSemaphore, fTools.readyToPresentSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
            SubmitCommandBuffer(m_graphicsQueue.handle, fTools.commandBuffer, 2, waitSemaphores, 1, &signalSemaphore, fTools.inFlightFence.handle);

            Present(m_device, m_graphicsQueue.handle, &m_swapchainValues.m_handle, 1, 1, &fTools.readyToPresentSemaphore.handle, &swapchainIdx);
        }

        else
        {
            // The command buffer recording begin here (stops when submit is called)
            BeginCommandBuffer(fTools.commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // The viewport and scissor are dynamic, so they should be set here
            DefineViewportAndScissor(fTools.commandBuffer, m_swapchainValues.m_extent);
            // Attachment barriers for layout transitions before rendering
            VkImageMemoryBarrier2 renderPassBarriers[2] = {};
            ImageMemoryBarrier(readWrites.m_colorTarget.m_image.m_image.m_handle, renderPassBarriers[0], VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, colorAttachmentWorkingLayout,
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            ImageMemoryBarrier(readWrites.m_depthTarget.m_image.m_image.m_handle, renderPassBarriers[1], VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
            // execute
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, BLIT_ARRAY_SIZE(renderPassBarriers), renderPassBarriers);

            if (context.m_renders.m_renderCount == 0)
            {
                // TODO: Change this so that it instantly goes to present and quits the function before going further
                DrawBackgroundImage(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);
            }
            /*
                !RENDER OPERATIONS INFO:
                1.The first culling shader is called.
                  It only works on objects that were visible last frame and are not transparent.
                  It performs frustum culling and LOD selection.(See InitialDrawCull.comp)

                2.The first draw pass is called.
                  It takes the indirect commands and indirect count that were written by the culling shader.
                  It uses one draw call to draw everything that those buffer specify

                Pre-3.The depth generation shader is called, to allow for occlusion culling.

                3.The second culling shader is called.
                  It does frustum culling and LOD selection on every object. It also does occlusion culling this time.
                  It only creates indirect draw commands for the objects that were NOT visible last frame.
                  It also updates the visibility buffer for every object, to affect the next frame.
                  Transparent objects are ignored

                4.The second draw pass is called.
                  It is the exact same as the first one, but gets its commands from the second culling pass.

                5.The 3rd culling shader is called.
                  It is the same shader as the second pass but this time ignores opaque objects and operator on transparent ones.

                6.The final draw pass is called.
                  It takes the commands from the 3rd culling shader.
                  Its fragment shader also has a modified specialization constant for alpha discard
            */

            uint8_t latePass{ 0 };

            // First culling pass
            DrawCullFirstPass(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            // First draw pass
            DrawOpaque(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame, 
                { m_drawWidth, m_drawHeight }, latePass);

            latePass = 1;

            // Depth pyramid generation
            GenerateHiZ(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            // Second culling pass 
            DrawCullOcclusionPass(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            // Second draw pass
            DrawOpaque(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame,
                {m_drawWidth, m_drawHeight}, latePass);

            if (context.m_renders.m_transparentRenderCount != 0)
            {
                DrawCullTrans(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

                DrawTransparents(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame, 
                    {m_drawWidth, m_drawHeight});
            }

            /*
            Presentation:
            -The color attachment is copied to the current swapchain image
            -The commands are submitted
            -The swapchain image is presented
            */

            // Image barriers to transition the layout of the color attachment and the swapchain image
            VkImageMemoryBarrier2 presentBarriers[2] = {};
            ImageMemoryBarrier(readWrites.m_colorTarget.m_image.m_image.m_handle, presentBarriers[0], VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, colorAttachmentWorkingLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            ImageMemoryBarrier(m_swapchainValues.m_images[size_t(swapchainIdx)], presentBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            // Execute
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, BLIT_ARRAY_SIZE(presentBarriers), presentBarriers);

            // Copies the color attachment to the swapchain image
            if constexpr (BlitzenCore::Ce_DepthPyramidDebug)
            {
                CopyPyramidToSwapchain(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame, 
                    m_swapchainValues, swapchainIdx, m_drawWidth, m_drawHeight, context.m_camera.transformData.debugPyramidLevel);
            }
            else
            {
                CopyToSwapchain(fTools.commandBuffer, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame, 
                    m_swapchainValues, swapchainIdx);
            }
            

            // Adds semaphores and submits command buffer
            VkSemaphoreSubmitInfo waitSemaphores[2]{ {}, {} };
            CreateSemahoreSubmitInfo(waitSemaphores[0], fTools.imageAcquiredSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            CreateSemahoreSubmitInfo(waitSemaphores[1], fTools.buffersReadySemaphore.handle, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
            VkSemaphoreSubmitInfo signalSemaphore{};
            CreateSemahoreSubmitInfo(signalSemaphore, fTools.readyToPresentSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
            SubmitCommandBuffer(m_graphicsQueue.handle, fTools.commandBuffer, 2, waitSemaphores, 1, &signalSemaphore, fTools.inFlightFence.handle);

            Present(m_device, m_graphicsQueue.handle, &m_swapchainValues.m_handle, 1, 1, &fTools.readyToPresentSemaphore.handle, &swapchainIdx);
        }

        m_currentFrame = (m_currentFrame + 1) % ce_framesInFlight;
    }

    void VulkanRenderer::DrawWhileWaiting(float deltaTime)
    {
        auto& fTools = m_frameToolsList[0];
        auto colorAttachmentWorkingLayout = VK_IMAGE_LAYOUT_GENERAL;

        vkWaitForFences(m_device, 1, &fTools.inFlightFence.handle, VK_TRUE, ce_fenceTimeout);
        VK_CHECK(vkResetFences(m_device, 1, &(fTools.inFlightFence.handle)));

        // Swapchain image, needed to present the color attachment results
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_swapchainValues.m_handle, ce_swapchainImageTimeout, fTools.imageAcquiredSemaphore.handle, VK_NULL_HANDLE, &swapchainIdx);
        auto swapchainImage{ m_swapchainValues.m_images[swapchainIdx] };
        auto swapchainImageView{ m_swapchainValues.m_views[swapchainIdx] };

        // The command buffer recording begin here (stops when submit is called)
        BeginCommandBuffer(m_idleDrawCommandBuffer, 0);

        // The viewport and scissor are dynamic, so they should be set here
        DefineViewportAndScissor(m_idleDrawCommandBuffer, m_swapchainValues.m_extent);

        // Attachment barriers for layout transitions before rendering
        VkImageMemoryBarrier2 colorAttachmentDefinitionBarrier{};
        ImageMemoryBarrier(swapchainImage, colorAttachmentDefinitionBarrier, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            colorAttachmentWorkingLayout, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(m_idleDrawCommandBuffer, 0, nullptr, 0, nullptr, 1, &colorAttachmentDefinitionBarrier);

        VkRenderingAttachmentInfo colorAttachmentInfo{};
        CreateRenderingAttachmentInfo(colorAttachmentInfo, swapchainImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE, { 0.1f, 0.2f, 0.3f, 0 });
        BeginRendering(m_idleDrawCommandBuffer, m_swapchainValues.m_extent, { 0, 0 }, 1, &colorAttachmentInfo, nullptr, nullptr);

        vkCmdBindPipeline(m_idleDrawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.m_trianglePso.handle);

        m_pipelines.m_loadingTriangleVertexColor *= cos(deltaTime);
        vkCmdPushConstants(m_idleDrawCommandBuffer, m_pipelines.m_triangleLayout.handle, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BlitML::vec3), &m_pipelines.m_loadingTriangleVertexColor);

        vkCmdDraw(m_idleDrawCommandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(m_idleDrawCommandBuffer);

        // Create a barrier for the swapchain image to transition to present optimal
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(swapchainImage, presentImageBarrier,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // exectute
        PipelineBarrier(m_idleDrawCommandBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);

        VkSemaphoreSubmitInfo waitSemaphores{};
        CreateSemahoreSubmitInfo(waitSemaphores, fTools.imageAcquiredSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        VkSemaphoreSubmitInfo signalSemaphore{};
        CreateSemahoreSubmitInfo(signalSemaphore, fTools.readyToPresentSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

        SubmitCommandBuffer(m_graphicsQueue.handle, m_idleDrawCommandBuffer, 1, &waitSemaphores, 1, &signalSemaphore, fTools.inFlightFence.handle);

        Present(m_device, m_graphicsQueue.handle, &m_swapchainValues.m_handle, 1, 1, &fTools.readyToPresentSemaphore.handle, &swapchainIdx);
    }
}