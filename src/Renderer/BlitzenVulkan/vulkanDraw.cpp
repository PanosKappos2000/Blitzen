#include "Renderer/BlitzenVulkan/Context/vulkanRenderer.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vulkanCommands.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vkRuntimeHelpers.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanPipelines.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanRNDResources.h"
#include "Core/Events/blitTimeManager.h"
#include "BlitzenMathLibrary/blitML.h"

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
    }
}