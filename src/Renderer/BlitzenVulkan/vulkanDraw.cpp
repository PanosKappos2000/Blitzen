#include "Renderer/BlitzenVulkan/Context/vulkanRenderer.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vulkanCommands.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanPipelines.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanRNDResources.h"
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

    // Fallback to glm function
    static glm::vec4 glm_NormalizePlane(glm::vec4& plane)
    {
        return plane / glm::length(glm::vec3(plane));
    }

    static void UpdateBuffers(BlitzenEngine::DrawContext& context, CommandContext& cmdContext, RWResources& buffers, VkQueue queue)
    {
        BeginCommandBuffer(cmdContext.m_transferCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        CopyBufferToBuffer(cmdContext.m_transferCmdB, buffers.m_transformBuffer.m_staging.m_buffer.m_handle, buffers.m_transformBuffer.m_buffer.m_buffer.m_handle, 
            buffers.m_transformBuffer.m_staging.m_dataSize, 0, 0);

        // VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT is used here because the signal comes from a transfer queue.
        // More specific shader stages (like VERTEX or COMPUTE) are invalid for transfer queues per Vulkan spec.
        // This ensures compatibility with graphics queue work that reads the transform buffer.
        // DO NOT WASTE TIME TRYING TO CHANGE THIS
        VkSemaphoreSubmitInfo bufferCopySemaphoreInfo{};
        CreateSemahoreSubmitInfo(bufferCopySemaphoreInfo, cmdContext.m_bufferUpdateSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
        SubmitCommandBuffer(queue, cmdContext.m_transferCmdB, 0, nullptr, 1, &bufferCopySemaphoreInfo, VK_NULL_HANDLE);
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
        vkCmdPushConstants(cmdb, pipelineContext.m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &drawContext.m_pResidents->m_renders.m_opaqueStaticCount);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawContext.m_pResidents->m_renders.m_opaqueStaticCount, 64), 1, 1);

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

        descriptorContext.m_HI_Z_descInfo[frame].imageView = readWrites.m_HI_Z_MAP.m_pyramid.m_view.m_handle;

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
        vkCmdPushConstants(cmdb, pipelineContext.m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &drawContext.m_pResidents->m_renders.m_opaqueStaticCount);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawContext.m_pResidents->m_renders.m_opaqueStaticCount, 64), 1, 1);

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
        vkCmdPushConstants(cmdb, pipelineContext.m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &drawContext.m_pResidents->m_renders.m_transparentStaticCount);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawContext.m_pResidents->m_renders.m_transparentStaticCount, 64), 1, 1);

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

    static void DrawTemporalOcclusionPass(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
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
        VkBufferMemoryBarrier2 cullingBarriers[2]{};
        // Count reset barrier
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, cullingBarriers[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Commands read barrier
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, cullingBarriers[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);

        // Additional image memory barrier for depth pyramid
        VkImageMemoryBarrier2 HI_Z_barrier{};
        ImageMemoryBarrier(readWrites.m_HI_Z_MAP.m_pyramid.m_image.m_handle, HI_Z_barrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

        // execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers, 1, &HI_Z_barrier);

        descriptorContext.m_HI_Z_descInfo[frame].imageView = readWrites.m_HI_Z_MAP.m_pyramid.m_view.m_handle;

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_CullDescriptorCount,
            &descriptorContext.m_pushDescriptorsCull[Ce_CullDescriptorCount * frame]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[Ce_SharedDescriptorCount * frame]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, 1,
            &descriptorContext.m_HI_Z_cullDescriptor[frame]);

        // Pipeline and descriptors
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawTemporalOccPso.handle);
        vkCmdPushConstants(cmdb, pipelineContext.m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &drawContext.m_pResidents->m_renders.m_opaqueStaticCount);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawContext.m_pResidents->m_renders.m_opaqueStaticCount, 64), 1, 1);

        // Barrier blocks graphics command and count read
        VkBufferMemoryBarrier2 graphicsBarrier[2]{};
        // Count
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, graphicsBarrier[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Commands
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, graphicsBarrier[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(graphicsBarrier), graphicsBarrier, 0, nullptr);
    }

    static void DrawTemporalOccTrans(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
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
        VkBufferMemoryBarrier2 cullingBarriers[2]{};
        // Count reset barrier
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, cullingBarriers[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Commands read barrier
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, cullingBarriers[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);

        // Additional image memory barrier for depth pyramid
        VkImageMemoryBarrier2 HI_Z_barrier{};
        ImageMemoryBarrier(readWrites.m_HI_Z_MAP.m_pyramid.m_image.m_handle, HI_Z_barrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

        // execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers, 1, &HI_Z_barrier);

        descriptorContext.m_HI_Z_descInfo[frame].imageView = readWrites.m_HI_Z_MAP.m_pyramid.m_view.m_handle;

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_CullDescriptorCount,
            &descriptorContext.m_pushDescriptorsCull[Ce_CullDescriptorCount * frame]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[Ce_SharedDescriptorCount * frame]);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawCullLayout.handle, Ce_PushDescriptorSetID, 1,
            &descriptorContext.m_HI_Z_cullDescriptor[frame]);

        // Pipeline and descriptors
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineContext.m_drawTemporalOccPso.handle);
        vkCmdPushConstants(cmdb, pipelineContext.m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &drawContext.m_pResidents->m_renders.m_transparentStaticCount);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawContext.m_pResidents->m_renders.m_transparentStaticCount, 64), 1, 1);

        // Barrier blocks graphics command and count read
        VkBufferMemoryBarrier2 graphicsBarrier[2]{};
        // Count
        BufferMemoryBarrier(readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, graphicsBarrier[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Commands
        BufferMemoryBarrier(readWrites.m_drawCmdBuffer.m_buffer.m_handle, graphicsBarrier[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(graphicsBarrier), graphicsBarrier, 0, nullptr);
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
        vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &drawContext.m_pResidents->m_renders.m_opaqueStaticCount);
        // Dispatch
        vkCmdDispatch(cmdb, (drawContext.m_pResidents->m_renders.m_opaqueStaticCount / 64) + 1, 1, 1);

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
        vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &drawContext.m_pResidents->m_renders.m_transparentStaticCount);
        // Dispatch
        vkCmdDispatch(cmdb, (drawContext.m_pResidents->m_renders.m_transparentStaticCount / 64) + 1, 1, 1);

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
        vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &dispatchCount);
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
        vkCmdPushConstants(cmdb, pipelineContext.m_clusterCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &dispatchCount);
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

    static void RenderPassClear(VkCommandBuffer cmdb, PipelineContext& pipelineContext, uint32_t frame, VkExtent2D drawExtent)
    {
        pipelineContext.m_colorTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        pipelineContext.m_depthTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        BeginRendering(cmdb, drawExtent, { 0, 0 }, 1, &pipelineContext.m_colorTargetInfo[frame], &pipelineContext.m_depthTargetInfo[frame], nullptr);
    }

    static void RenderPassStore(VkCommandBuffer cmdb, PipelineContext& pipelineContext, uint32_t frame, VkExtent2D drawExtent)
    {
        pipelineContext.m_colorTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        pipelineContext.m_depthTargetInfo[frame].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        BeginRendering(cmdb, drawExtent, { 0, 0 }, 1, &pipelineContext.m_colorTargetInfo[frame], &pipelineContext.m_depthTargetInfo[frame], nullptr);
    }

    static void DrawOpaque(VkCommandBuffer cmdb, VkInstance instance, PipelineContext& pipelineContext, ROResources& readOnlies,
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame)
    {
        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_PushDescriptorSetID, Ce_GraphicsDescriptorCount,
            descriptorContext.m_pushDescriptorsGraphics);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[frame * Ce_SharedDescriptorCount]);
        vkCmdBindDescriptorSets(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_TextureDescriptorsSetID, 1,
            &descriptorContext.m_textureDescriptorSet, 0, nullptr);

        // Draw
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawPso.handle);
        vkCmdBindIndexBuffer(cmdb, readOnlies.m_idxBuffer.m_buffer.m_handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirectCount(cmdb, readWrites.m_drawCmdBuffer.m_buffer.m_handle, offsetof(IndirectDrawData, drawIndirect),
            readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, Ce_DrawCmdElementCount, sizeof(IndirectDrawData));
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
        RWResources& readWrites, DescriptorContext& descriptorContext, BlitzenEngine::DrawContext& drawContext, uint32_t frame)
    {
        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_PushDescriptorSetID, Ce_GraphicsDescriptorCount,
            descriptorContext.m_pushDescriptorsGraphics);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_PushDescriptorSetID, Ce_SharedDescriptorCount,
            &descriptorContext.m_pushDescriptorsShared[frame * Ce_SharedDescriptorCount]);
        vkCmdBindDescriptorSets(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_opaqueDrawLayout.handle, Ce_TextureDescriptorsSetID, 1, 
            &descriptorContext.m_textureDescriptorSet, 0, nullptr);

        // Draw
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineContext.m_tranparentDrawPso.handle);
        vkCmdBindIndexBuffer(cmdb, readOnlies.m_idxBuffer.m_buffer.m_handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirectCount(cmdb, readWrites.m_drawCmdBuffer.m_buffer.m_handle, offsetof(IndirectDrawData, drawIndirect),
            readWrites.m_drawCmdCounterBuffer.m_buffer.m_handle, 0, Ce_DrawCmdElementCount, sizeof(IndirectDrawData));
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

    void VulkanRenderer::DrawFrame(BlitzenEngine::DrawContext& context)
    {
        auto& cmd = m_commandsContext[m_currentFrame];
        auto& readWrites = m_readWrites[m_currentFrame];

        // Waits for previous rendering commands
        vkWaitForFences(m_device, 1, &cmd.m_frameFence.handle, VK_TRUE, ce_fenceTimeout);
        VK_CHECK_MSG(vkResetFences(m_device, 1, &cmd.m_frameFence.handle));

        UpdateBuffers(context, cmd, readWrites, m_transferQueue.handle);
        if (context.m_camera.transformData.bFreezeFrustum)
        {
            // Only change the matrix that moves the camera if the freeze frustum debug functionality is active
            readWrites.m_viewDataBuffer.m_pMapped->projectionViewMatrix = context.m_camera.viewData.projectionViewMatrix;
        }
        else
        {
            *(readWrites.m_viewDataBuffer.m_pMapped) = context.m_camera.viewData;
        }

        // Swapchain image
        vkAcquireNextImageKHR(m_device, m_swapchain.m_handle, ce_swapchainImageTimeout, cmd.m_swapchainSemaphore.handle, VK_NULL_HANDLE, &m_swapchainIDX);

        if constexpr (BlitzenCore::Ce_BuildClusters)
        {
            // Fist culling pass with separate command buffer
            BeginCommandBuffer(cmd.m_computeCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // Generates cluster dispatch data and count for the opaque render objects
            ClusterDispatch(cmd.m_computeCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            if (context.m_pResidents->m_renders.m_transparentStaticCount != 0)
            {
                ClusterCullDispatchTrans(cmd.m_computeCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);
            }

            // Submits command buffer to generate cluster dispatch count
            VkSemaphoreSubmitInfo bufferUpdateWaitSemaphore{};
            CreateSemahoreSubmitInfo(bufferUpdateWaitSemaphore, cmd.m_bufferUpdateSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

            VkSemaphoreSubmitInfo waitForClusterData{};
            CreateSemahoreSubmitInfo(waitForClusterData, cmd.m_clusterSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            SubmitCommandBuffer(m_computeQueue.handle, cmd.m_computeCmdB, 1, &bufferUpdateWaitSemaphore, 1, &waitForClusterData, cmd.m_preClusterFence.handle);

            // FENCE DISPATCH
            vkWaitForFences(m_device, 1, &cmd.m_preClusterFence.handle, VK_TRUE, ce_fenceTimeout);
            vkResetFences(m_device, 1, &cmd.m_preClusterFence.handle);

            // Command recording begins again
            BeginCommandBuffer(cmd.m_mainGraphicsCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            DefineViewportAndScissor(cmd.m_mainGraphicsCmdB, m_swapchain.m_extent);

            uint32_t dispatchCount{ uint32_t(*reinterpret_cast<uint32_t*>(readWrites.m_clusterDispatchCounterCopy.m_buffer.m_vmaInfo.pMappedData)) };
            uint32_t transparentDispatchCount = 0;
            if (false)//context.m_pResidents->m_renders.m_transparentStaticCount)
            {
                transparentDispatchCount = uint32_t(*reinterpret_cast<uint32_t*>(readWrites.m_transClusterDispatchCounterCopy.m_buffer.m_vmaInfo.pMappedData));
            }

            // Culls opaque render object clusters
            ClusterCull(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame, dispatchCount);

            FirstRenderPassBarriers(cmd.m_mainGraphicsCmdB, readWrites.m_colorTarget.m_image.m_image.m_handle, readWrites.m_depthTarget.m_image.m_image.m_handle);

            RenderPassClear(cmd.m_mainGraphicsCmdB, m_pipelines, m_currentFrame, VkExtent2D{ m_drawWidth, m_drawHeight });

            DrawOpaque(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            vkCmdEndRendering(cmd.m_mainGraphicsCmdB);

            if (context.m_pResidents->m_renders.m_transparentStaticCount != 0)
            {
                ClusterCullTransparent(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame],
                    m_descriptorContext, context, m_currentFrame, transparentDispatchCount);

                RenderPassStore(cmd.m_mainGraphicsCmdB, m_pipelines, m_currentFrame, VkExtent2D{ m_drawWidth, m_drawHeight });

                DrawTransparents(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

                vkCmdEndRendering(cmd.m_mainGraphicsCmdB);
            }

            // Copies the color attachment to the swapchain image
            CopyTargetToSwapchain(cmd.m_mainGraphicsCmdB);

            VkSemaphoreSubmitInfo waitSemaphores[2]{ {}, {} };
            CreateSemahoreSubmitInfo(waitSemaphores[0], cmd.m_swapchainSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            CreateSemahoreSubmitInfo(waitSemaphores[1], cmd.m_clusterSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

            VkSemaphoreSubmitInfo signalSemaphore{};
            CreateSemahoreSubmitInfo(signalSemaphore, cmd.m_renderSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

            SubmitCommandBuffer(m_graphicsQueue.handle, cmd.m_mainGraphicsCmdB, 2, waitSemaphores, 1, &signalSemaphore, cmd.m_frameFence.handle);
        }

        else if constexpr (BlitzenCore::Ce_DrawTemporalOcclusion)
        {
            BeginCommandBuffer(cmd.m_mainGraphicsCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            DefineViewportAndScissor(cmd.m_mainGraphicsCmdB, m_swapchain.m_extent);

            FirstRenderPassBarriers(cmd.m_mainGraphicsCmdB, readWrites.m_colorTarget.m_image.m_image.m_handle, readWrites.m_depthTarget.m_image.m_image.m_handle);

            GenerateHiZ(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            /*if (context.m_renders.m_transparentRenderCount != 0)
            {
                DrawTemporalOccTrans(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

                RenderPassStore(cmd.m_mainGraphicsCmdB, m_pipelines, m_currentFrame, VkExtent2D{ m_drawWidth, m_drawHeight });

                DrawTransparents(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame,
                    { m_drawWidth, m_drawHeight });

                vkCmdEndRendering(cmd.m_mainGraphicsCmdB);
            }*/

            DrawTemporalOcclusionPass(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            RenderPassClear(cmd.m_mainGraphicsCmdB, m_pipelines, m_currentFrame, VkExtent2D{ m_drawWidth, m_drawHeight });

            DrawOpaque(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            vkCmdEndRendering(cmd.m_mainGraphicsCmdB);

            // COPIES COLOR TARGET TO SWAPCHAIN
            if constexpr (BlitzenCore::Ce_DepthPyramidDebug)
            {
                CopyPyramidToSwapchain(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame,
                    m_swapchain, m_swapchainIDX, m_drawWidth, m_drawHeight, context.m_camera.transformData.debugPyramidLevel);
            }
            else
            {
                CopyTargetToSwapchain(cmd.m_mainGraphicsCmdB);
            }

            // SUBMIT
            VkSemaphoreSubmitInfo waitSemaphores[2]{ {}, {} };
            CreateSemahoreSubmitInfo(waitSemaphores[0], cmd.m_swapchainSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            CreateSemahoreSubmitInfo(waitSemaphores[1], cmd.m_bufferUpdateSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

            VkSemaphoreSubmitInfo signalSemaphore{};
            CreateSemahoreSubmitInfo(signalSemaphore, cmd.m_renderSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

            SubmitCommandBuffer(m_graphicsQueue.handle, cmd.m_mainGraphicsCmdB, 2, waitSemaphores, 1, &signalSemaphore, cmd.m_frameFence.handle);
        }
        else
        {
            BeginCommandBuffer(cmd.m_mainGraphicsCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            DefineViewportAndScissor(cmd.m_mainGraphicsCmdB, m_swapchain.m_extent);

            uint8_t latePass{ 0 };

            // CULLING NO OCCLUDERS
            DrawCullFirstPass(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            FirstRenderPassBarriers(cmd.m_mainGraphicsCmdB, readWrites.m_colorTarget.m_image.m_image.m_handle, readWrites.m_depthTarget.m_image.m_image.m_handle);

            // FIRST RENDER PASS(OCCLUDER GEN)
            RenderPassClear(cmd.m_mainGraphicsCmdB, m_pipelines, m_currentFrame, VkExtent2D{ m_drawWidth, m_drawHeight });
            DrawOpaque(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);
            vkCmdEndRendering(cmd.m_mainGraphicsCmdB);

            // HI_Z FOR OCCLUSION, AFTER PASS
            GenerateHiZ(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            // CULLING WITH OCCLUDERS 
            DrawCullOcclusionPass(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

            // SECOND RENDER PASS(OCCLUSION CULLED SCENE)
            RenderPassStore(cmd.m_mainGraphicsCmdB, m_pipelines, m_currentFrame, VkExtent2D{ m_drawWidth, m_drawHeight });
            DrawOpaque(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);
            vkCmdEndRendering(cmd.m_mainGraphicsCmdB);

            if (false)//context.m_pResidents->m_renders.m_transparentStaticCount != 0)
            {
                // TRANSPARENT CULLING (takes advantage of already generated scene)
                DrawCullTrans(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);

                // THIRD RENDER PASS(transparent objects)
                RenderPassStore(cmd.m_mainGraphicsCmdB, m_pipelines, m_currentFrame, VkExtent2D{ m_drawWidth, m_drawHeight });
                DrawTransparents(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame);
                vkCmdEndRendering(cmd.m_mainGraphicsCmdB);
            }

            // COPIES COLOR TARGET TO SWAPCHAIN
            if constexpr (BlitzenCore::Ce_DepthPyramidDebug)
            {
                CopyPyramidToSwapchain(cmd.m_mainGraphicsCmdB, m_instance, m_pipelines, m_readOnlies, m_readWrites[m_currentFrame], m_descriptorContext, context, m_currentFrame,
                    m_swapchain, m_swapchainIDX, m_drawWidth, m_drawHeight, context.m_camera.transformData.debugPyramidLevel);
            }
            else
            {
                CopyTargetToSwapchain(cmd.m_mainGraphicsCmdB);
            }
            
            // SUBMIT
            VkSemaphoreSubmitInfo waitSemaphores[2]{ {}, {} };
            CreateSemahoreSubmitInfo(waitSemaphores[0], cmd.m_swapchainSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            CreateSemahoreSubmitInfo(waitSemaphores[1], cmd.m_bufferUpdateSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

            VkSemaphoreSubmitInfo signalSemaphore{};
            CreateSemahoreSubmitInfo(signalSemaphore, cmd.m_renderSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

            SubmitCommandBuffer(m_graphicsQueue.handle, cmd.m_mainGraphicsCmdB, 2, waitSemaphores, 1, &signalSemaphore, cmd.m_frameFence.handle);
        }
    }

    void VulkanRenderer::DrawWhileWaiting(float deltaTime)
    {
        auto& cmd = m_commandsContext[0];
        auto colorAttachmentWorkingLayout = VK_IMAGE_LAYOUT_GENERAL;

        vkWaitForFences(m_device, 1, &cmd.m_uiFence.handle, VK_TRUE, ce_fenceTimeout);
        VK_CHECK_MSG(vkResetFences(m_device, 1, &cmd.m_uiFence.handle));

        vkAcquireNextImageKHR(m_device, m_swapchain.m_handle, ce_swapchainImageTimeout, cmd.m_swapchainSemaphore.handle, VK_NULL_HANDLE, &m_swapchainIDX);
        auto swapchainImage{ m_swapchain.m_images[m_swapchainIDX] };
        auto swapchainImageView{ m_swapchain.m_views[m_swapchainIDX] };

        // The command buffer recording begin here (stops when submit is called)
        BeginCommandBuffer(cmd.m_uiGraphicsCmdBuffer, 0);

        // The viewport and scissor are dynamic, so they should be set here
        DefineViewportAndScissor(cmd.m_uiGraphicsCmdBuffer, m_swapchain.m_extent);

        // Attachment barriers for layout transitions before rendering
        VkImageMemoryBarrier2 colorAttachmentDefinitionBarrier{};
        ImageMemoryBarrier(swapchainImage, colorAttachmentDefinitionBarrier, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            colorAttachmentWorkingLayout, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(cmd.m_uiGraphicsCmdBuffer, 0, nullptr, 0, nullptr, 1, &colorAttachmentDefinitionBarrier);

        VkRenderingAttachmentInfo colorAttachmentInfo{};
        CreateRenderingAttachmentInfo(colorAttachmentInfo, swapchainImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE, { 0.1f, 0.2f, 0.3f, 0 });
        BeginRendering(cmd.m_uiGraphicsCmdBuffer, m_swapchain.m_extent, { 0, 0 }, 1, &colorAttachmentInfo, nullptr, nullptr);

        vkCmdBindPipeline(cmd.m_uiGraphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.m_trianglePso.handle);

        m_pipelines.m_loadingTriangleVertexColor *= cos(deltaTime);
        vkCmdPushConstants(cmd.m_uiGraphicsCmdBuffer, m_pipelines.m_triangleLayout.handle, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BlitML::vec3), &m_pipelines.m_loadingTriangleVertexColor);

        vkCmdDraw(cmd.m_uiGraphicsCmdBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(cmd.m_uiGraphicsCmdBuffer);

        // Create a barrier for the swapchain image to transition to present optimal
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(swapchainImage, presentImageBarrier,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // exectute
        PipelineBarrier(cmd.m_uiGraphicsCmdBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);

        VkSemaphoreSubmitInfo waitSemaphores{};
        CreateSemahoreSubmitInfo(waitSemaphores, cmd.m_swapchainSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        VkSemaphoreSubmitInfo signalSemaphore{};
        CreateSemahoreSubmitInfo(signalSemaphore, cmd.m_renderSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

        SubmitCommandBuffer(m_graphicsQueue.handle, cmd.m_uiGraphicsCmdBuffer, 1, &waitSemaphores, 1, &signalSemaphore, cmd.m_uiFence.handle);

        Present(1);
    }
}