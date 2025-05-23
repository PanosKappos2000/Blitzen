#include "vulkanRenderer.h"
#include "vulkanCommands.h"
#include "vulkanPipelines.h"
#include "vulkanResourceFunctions.h"
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
    static void DrawMeshTasks(VkInstance instance, VkCommandBuffer commandBuffer, VkBuffer drawBuffer,
        VkDeviceSize drawOffset, VkBuffer countBuffer, VkDeviceSize countOffset, uint32_t maxDrawCount, uint32_t stride)
    {
        auto func = (PFN_vkCmdDrawMeshTasksIndirectCountEXT)vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectCountEXT");
        if (func != nullptr)
        {
            func(commandBuffer, drawBuffer, drawOffset, countBuffer, countOffset, maxDrawCount, stride);
        }
    }

    // Call vkCmdPushDescriptorSetKHR extension function (This can be removed if I upgrade to Vulkan 1.4)
    static void PushDescriptors(VkInstance instance, VkCommandBuffer commandBuffer,
        VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t set,
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

    static void UpdateBuffers(BlitzenEngine::DrawContext& context, VulkanRenderer::FrameTools& tools,
        VulkanRenderer::VarBuffers& buffers, VkQueue queue)
    {
        BeginCommandBuffer(tools.transferCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        CopyBufferToBuffer(tools.transferCommandBuffer, buffers.transformStagingBuffer.bufferHandle,
            buffers.transformBuffer.buffer.bufferHandle, buffers.dynamicTransformDataSize, 0, 0);

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
    static void DrawCullFirstPass(VkCommandBuffer cmdb, VkInstance instance, VkPipeline pipeline, VkPipelineLayout layout,
        VulkanRenderer::StaticBuffers& staticBuffers, VulkanRenderer::VarBuffers& varBuffers, 
        uint32_t drawCount, uint32_t descriptorCount, VkWriteDescriptorSet* pDescriptors, VkDeviceAddress objAddress)
    {
        // Count reset barrier
        VkBufferMemoryBarrier2 waitBeforeZeroingCountBuffer{};
        BufferMemoryBarrier(staticBuffers.indirectCountBuffer.buffer.bufferHandle, waitBeforeZeroingCountBuffer, 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, 
            VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Execute 
        PipelineBarrier(cmdb, 0, nullptr, 1, &waitBeforeZeroingCountBuffer, 0, nullptr);
        // Reset
        vkCmdFillBuffer(cmdb, staticBuffers.indirectCountBuffer.buffer.bufferHandle, 0, sizeof(uint32_t), 0);

        // Barrier waits for count reset, last frame draw commands read and visibility buffer write
        VkBufferMemoryBarrier2 dispatchBarriers[3]{};
        // Count reset barrier
        BufferMemoryBarrier(staticBuffers.indirectCountBuffer.buffer.bufferHandle, dispatchBarriers[0],
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
            VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Commands read barrier
        BufferMemoryBarrier(staticBuffers.indirectDrawBuffer.buffer.bufferHandle, dispatchBarriers[1],
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Visibility buffer Barrier
        BufferMemoryBarrier(staticBuffers.visibilityBuffer.buffer.bufferHandle, dispatchBarriers[2], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(dispatchBarriers), dispatchBarriers, 0, nullptr);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, layout, PushDescriptorSetID, descriptorCount, pDescriptors);

        // Pipeline and descriptors
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        DrawCullShaderPushConstant pushConstant{ objAddress, drawCount };
        vkCmdPushConstants(cmdb, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DrawCullShaderPushConstant), &pushConstant);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawCount, 64), 1, 1);

        // Barrier blocks graphics command and count read
        VkBufferMemoryBarrier2 waitForCullingShader[2] = {};
        // Count
        BufferMemoryBarrier(staticBuffers.indirectCountBuffer.buffer.bufferHandle, waitForCullingShader[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Commands
        BufferMemoryBarrier(staticBuffers.indirectDrawBuffer.buffer.bufferHandle, waitForCullingShader[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, 
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(waitForCullingShader), waitForCullingShader, 0, nullptr);
    }

    // Prepares the second culling compute pass (frustum culling, LOD selection, occlusion, visibility setting)
    // Creates commands for previously culled objects. Sets visibility for non-culled objects (already drawn after previous pass)
    static void DrawCullOcclusionPass(VkCommandBuffer cmdb, VkInstance instance, VkPipeline pipeline, VkPipelineLayout layout,
        VulkanRenderer::StaticBuffers& staticBuffers, VulkanRenderer::VarBuffers& varBuffers, PushDescriptorImage& depthPyramid, 
        PushDescriptorImage& depthAttachment, uint32_t drawCount, uint32_t descriptorCount, VkWriteDescriptorSet* pDescriptors, VkDeviceAddress objAddress)
    {
        // Depth pyramid update
        VkWriteDescriptorSet depthPyramidDescriptor{};
        VkDescriptorImageInfo depthPyramidDescriptorInfo{};
        WriteImageDescriptorSets(depthPyramidDescriptor, depthPyramidDescriptorInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE,
            Ce_DepthPyramidImageBindingID, VK_IMAGE_LAYOUT_GENERAL, depthPyramid.image.imageView, depthAttachment.sampler.handle);

        // Count reset barrier
        VkBufferMemoryBarrier2 waitBeforeZeroingCountBuffer{};
        BufferMemoryBarrier(staticBuffers.indirectCountBuffer.buffer.bufferHandle, waitBeforeZeroingCountBuffer,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Execute 
        PipelineBarrier(cmdb, 0, nullptr, 1, &waitBeforeZeroingCountBuffer, 0, nullptr);
        // Reset
        vkCmdFillBuffer(cmdb, staticBuffers.indirectCountBuffer.buffer.bufferHandle, 0, sizeof(uint32_t), 0);

        // Barrier waits for count reset, last frame draw commands read and visibility buffer read
        VkBufferMemoryBarrier2 dispatchBarriers[3]{};
        // Count reset barrier
        BufferMemoryBarrier(staticBuffers.indirectCountBuffer.buffer.bufferHandle, dispatchBarriers[0],
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Commands read barrier
        BufferMemoryBarrier(staticBuffers.indirectDrawBuffer.buffer.bufferHandle, dispatchBarriers[1],
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Visibility buffer Barrier
        BufferMemoryBarrier(staticBuffers.visibilityBuffer.buffer.bufferHandle, dispatchBarriers[2], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        // Additional image memory barrier for depth pyramid
        VkImageMemoryBarrier2 waitForDepthPyramidGeneration{};
        ImageMemoryBarrier(depthPyramid.image.image, waitForDepthPyramidGeneration, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(dispatchBarriers), dispatchBarriers, 1, &waitForDepthPyramidGeneration);

        // Descriptors
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, layout, PushDescriptorSetID, descriptorCount, pDescriptors);
        PushDescriptors(instance, cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, layout, PushDescriptorSetID, 1, &depthPyramidDescriptor);

        // Pipeline and descriptors
        vkCmdBindPipeline(cmdb, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        DrawCullShaderPushConstant pushConstant{ objAddress, drawCount };
        vkCmdPushConstants(cmdb, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(DrawCullShaderPushConstant), &pushConstant);

        // Dispatch
        vkCmdDispatch(cmdb, BlitML::GetComputeShaderGroupSize(drawCount, 64), 1, 1);

        // Barrier blocks graphics command and count read
        VkBufferMemoryBarrier2 waitForCullingShader[2] = {};
        // Count
        BufferMemoryBarrier(staticBuffers.indirectCountBuffer.buffer.bufferHandle, waitForCullingShader[0], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Commands
        BufferMemoryBarrier(staticBuffers.indirectDrawBuffer.buffer.bufferHandle, waitForCullingShader[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(cmdb, 0, nullptr, BLIT_ARRAY_SIZE(waitForCullingShader), waitForCullingShader, 0, nullptr);
    }

    // DEPRECATED __TODO: REMOVE
    static void DispatchRenderObjectCullingComputeShader(VkCommandBuffer commandBuffer,
        VkPipeline pipeline, VkPipelineLayout layout, uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites,
        VkBuffer indirectCountBuffer, VkBuffer indirectCommandsBuffer, VkBuffer visibilityBuffer,
        PushDescriptorImage& depthAttachment, PushDescriptorImage& depthPyramid,
        uint32_t drawCount, VkDeviceAddress renderObjectBufferAddress,
        uint8_t lateCulling, VkInstance instance)
    {
        // Adds depth pyramid image descriptor for occlusion culling shader
        if(lateCulling)
        { 
            VkWriteDescriptorSet depthPyramidWrite{};
            VkDescriptorImageInfo depthPyramidImageInfo{};
            WriteImageDescriptorSets(depthPyramidWrite, depthPyramidImageInfo, 
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE,
                Ce_DepthPyramidImageBindingID, VK_IMAGE_LAYOUT_GENERAL,
                depthPyramid.image.imageView, depthAttachment.sampler.handle);
            pDescriptorWrites[descriptorWriteCount - 1] = depthPyramidWrite;
        }

        // Indirect count buffer is set to zero, after the previous pipeline is done with it
        VkBufferMemoryBarrier2 waitBeforeZeroingCountBuffer{};
        BufferMemoryBarrier(indirectCountBuffer, waitBeforeZeroingCountBuffer,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_ACCESS_2_TRANSFER_WRITE_BIT,
             0, VK_WHOLE_SIZE);
        PipelineBarrier(commandBuffer, 0, nullptr, 1, &waitBeforeZeroingCountBuffer, 0, nullptr);
        vkCmdFillBuffer(commandBuffer, indirectCountBuffer, 0, sizeof(uint32_t), 0);
        
        // More synchronization
        VkBufferMemoryBarrier2 waitBeforeDispatchingShaders[3] = {};
        // Indirect count buffer waits for vkCmdFillBuffer to finish
        BufferMemoryBarrier(indirectCountBuffer, waitBeforeDispatchingShaders[0], 
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 
            0, VK_WHOLE_SIZE);
        // The indirect draw buffer waits for the indirect draw stage to finish reading commands
        BufferMemoryBarrier(indirectCommandsBuffer, waitBeforeDispatchingShaders[1], 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, 
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            0, VK_WHOLE_SIZE);
        // Visibility buffer barrier waits for previous shader to read or write from it
        BufferMemoryBarrier(visibilityBuffer, waitBeforeDispatchingShaders[2], 
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, 
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, 
        0, VK_WHOLE_SIZE);
        // For late culling an image barrier for depth pyramid generation is also placed
        if(lateCulling)
        {
            // This barrier waits for the depth generation compute shader to finish writing to the depth pyramid image
            VkImageMemoryBarrier2 waitForDepthPyramidGeneration{};
            ImageMemoryBarrier(depthPyramid.image.image, waitForDepthPyramidGeneration, 
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, // No layout transtions
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            PipelineBarrier(commandBuffer, 0, nullptr, 3, waitBeforeDispatchingShaders, 1, &waitForDepthPyramidGeneration);
        }
        // If this is the initial culling stage, simply adds the 2 barriers
        else
        {
            PipelineBarrier(commandBuffer, 0, nullptr, 3, waitBeforeDispatchingShaders, 0, nullptr);
        }

        // Dispatches the shader
        PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, PushDescriptorSetID, 
            lateCulling ? descriptorWriteCount : descriptorWriteCount - 1, pDescriptorWrites);
		// Binds the pipeline before push constants
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        DrawCullShaderPushConstant pushConstant{ renderObjectBufferAddress, drawCount };
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 
            0,sizeof(DrawCullShaderPushConstant), &pushConstant);
        vkCmdDispatch(commandBuffer, (drawCount / 64) + 1, 1, 1);

        // Stops the indirect stage from reading command and count, until the shader is done
        VkBufferMemoryBarrier2 waitForCullingShader[2] = {};
        BufferMemoryBarrier(indirectCountBuffer, waitForCullingShader[0], 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 
            0, VK_WHOLE_SIZE);
        BufferMemoryBarrier(indirectCommandsBuffer, waitForCullingShader[1], 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, 
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 
            0, VK_WHOLE_SIZE);
        PipelineBarrier(commandBuffer, 0, nullptr, BLIT_ARRAY_SIZE(waitForCullingShader),
            waitForCullingShader, 0, nullptr);
    }

    static void PreClusterDrawCull(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkPipelineLayout layout, uint32_t descriptorWriteCount, 
        VkWriteDescriptorSet* pDescriptorWrites, VkBuffer clusterCountBuffer, VkDeviceAddress clusterCountBufferAddress,
        VkBuffer clusterDataBuffer, VkDeviceAddress clusterDispatchBufferAddress,  VkBuffer clusterCountCopy, uint32_t drawCount, 
        VkDeviceAddress renderObjectBufferAddress, uint8_t lateCulling, VkInstance instance)
    {
        // Barrier before count reset
        VkBufferMemoryBarrier2 waitBeforeZeroingClusterCount{};
        BufferMemoryBarrier(clusterCountBuffer, waitBeforeZeroingClusterCount, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, sizeof(uint32_t));
        PipelineBarrier(commandBuffer, 0, nullptr, Ce_SinglePointer, &waitBeforeZeroingClusterCount, 0, nullptr);
        // Reset
        vkCmdFillBuffer(commandBuffer, clusterCountBuffer, 0, sizeof(uint32_t), 0);

        // Barrier for previous frame cluster count and cluster dispatch read
        VkBufferMemoryBarrier2 waitForBuffersBeforeDispatch[2]{};
        // Cluster count
        BufferMemoryBarrier(clusterCountBuffer, waitForBuffersBeforeDispatch[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 0, sizeof(uint32_t));
        // Cluster dispatch
        BufferMemoryBarrier(clusterDataBuffer, waitForBuffersBeforeDispatch[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 0, sizeof(ClusterDispatchData) * drawCount);
        // Execute
        PipelineBarrier(commandBuffer, 0, nullptr, BLIT_ARRAY_SIZE(waitForBuffersBeforeDispatch), waitForBuffersBeforeDispatch,
            0, nullptr);

        // Descriptors
        PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, PushDescriptorSetID, descriptorWriteCount, pDescriptorWrites);

		// Pipeline and push constants
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        ClusterCullShaderPushConstant pushConstant
        {
            renderObjectBufferAddress, clusterDispatchBufferAddress, 
            clusterCountBufferAddress, drawCount
        };
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ClusterCullShaderPushConstant), &pushConstant);
        // Dispatch
        vkCmdDispatch(commandBuffer, (drawCount / 64) + 1, 1, 1);

        // Cluster dispatch read barrier
        VkBufferMemoryBarrier2 clusterDispatchVisibilityBarrier{};
        BufferMemoryBarrier(clusterDataBuffer, clusterDispatchVisibilityBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        PipelineBarrier(commandBuffer, 0, nullptr, 1, &clusterDispatchVisibilityBarrier, 0, nullptr);

        // Cluster count copy barrier
        VkBufferMemoryBarrier2 copyToStagingBufferBarrier{};
        BufferMemoryBarrier(clusterCountBuffer, copyToStagingBufferBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, 0, sizeof(uint32_t));
        PipelineBarrier(commandBuffer, 0, nullptr, 1, &copyToStagingBufferBarrier, 0, nullptr);
        // Copy
        CopyBufferToBuffer(commandBuffer, clusterCountBuffer, clusterCountCopy, sizeof(uint32_t), 0, 0);
    }

    static void ClusterCull(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkPipelineLayout layout, uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites,
        VkBuffer clusterCountBuffer, AllocatedBuffer& clusterCountCopyBuffer, VkBuffer clusterDispatchBuffer, VkDeviceAddress clusterDispatchBufferAddress, 
        VkBuffer drawCountBuffer, VkBuffer indirectDrawBuffer, uint32_t dispatchCount, VkDeviceAddress renderObjectBufferAddress, VkInstance instance)
    {
        // Draw count reset barrier
        VkBufferMemoryBarrier2 drawCountFillBarrier{};
        BufferMemoryBarrier(drawCountBuffer, drawCountFillBarrier, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, sizeof(uint32_t));
        PipelineBarrier(commandBuffer, 0, nullptr, Ce_SinglePointer, &drawCountFillBarrier, 0, nullptr);
        vkCmdFillBuffer(commandBuffer, drawCountBuffer, 0, sizeof(uint32_t), 0);

        // Wait for draw count reset, previous frame command read and cluster dispatch write
        VkBufferMemoryBarrier2 waitBeforeDispatchingShaders[3] = {};
        // Draw count reset
        BufferMemoryBarrier(drawCountBuffer, waitBeforeDispatchingShaders[0], VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
            0, VK_WHOLE_SIZE);
        // Command read barrier
        BufferMemoryBarrier(indirectDrawBuffer, waitBeforeDispatchingShaders[1], VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            0, VK_WHOLE_SIZE);
        // Cluster dispatch barrier
        BufferMemoryBarrier(clusterDispatchBuffer, waitBeforeDispatchingShaders[2], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 0, VK_WHOLE_SIZE);
        // Execution
        PipelineBarrier(commandBuffer, 0, nullptr, BLIT_ARRAY_SIZE(waitBeforeDispatchingShaders), waitBeforeDispatchingShaders, 0, nullptr);

        // Descriptors
        PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, PushDescriptorSetID, descriptorWriteCount, pDescriptorWrites);
        
        // Pipeline and push constants
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        ClusterCullShaderPushConstant pushConstant
        { 
            renderObjectBufferAddress, clusterDispatchBufferAddress, 0, dispatchCount
        };
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ClusterCullShaderPushConstant), &pushConstant);
        // Dispatch
        vkCmdDispatch(commandBuffer, BlitML::GetComputeShaderGroupSize(dispatchCount, 64) , 1, 1);

        // Barriers stop graphics command read and count read
        VkBufferMemoryBarrier2 waitForCullingShader[2]{};
        // Count read
        BufferMemoryBarrier(drawCountBuffer, waitForCullingShader[0],VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 0, VK_WHOLE_SIZE);
        // Command read
        BufferMemoryBarrier(indirectDrawBuffer, waitForCullingShader[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
            0, VK_WHOLE_SIZE);
        // Execute
        PipelineBarrier(commandBuffer, 0, nullptr, BLIT_ARRAY_SIZE(waitForCullingShader), waitForCullingShader, 0, nullptr);
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

    static void DrawGeometry(VkCommandBuffer commandBuffer, VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorWriteCount,
        VkPipeline pipeline, VkPipelineLayout layout, VkDescriptorSet* textureSet, VkRenderingAttachmentInfo& colorAttachmentInfo, 
        VkRenderingAttachmentInfo& depthAttachmentInfo, VkExtent2D drawExtent, VulkanRenderer::StaticBuffers& staticBuffers,
        uint32_t drawCount, uint8_t latePass, VkInstance instance, uint8_t bRaytracing, uint32_t tlasCount, VkAccelerationStructureKHR* pTlas)
    {
        // Render pass begin
        colorAttachmentInfo.loadOp = latePass ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachmentInfo.loadOp = latePass ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        BeginRendering(commandBuffer, drawExtent, {0, 0}, 1, &colorAttachmentInfo, &depthAttachmentInfo, nullptr);

        // Raytracing
        VkWriteDescriptorSetAccelerationStructureKHR layoutAccelerationStructurePNext{};
        if(bRaytracing)
        {
            layoutAccelerationStructurePNext.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
            layoutAccelerationStructurePNext.accelerationStructureCount = tlasCount;
            layoutAccelerationStructurePNext.pAccelerationStructures = pTlas;
            //tlasBuffer.descriptorWrite.pNext = &layoutAccelerationStructurePNext;
            //PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, PushDescriptorSetID, 1, pDescriptorWrites);
        }

        // Descriptors
        PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, PushDescriptorSetID, descriptorWriteCount, pDescriptorWrites);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, TextureDescriptorSetID, 1, textureSet, 0, nullptr);

        // Push constants
        GlobalShaderDataPushConstant pcData{ staticBuffers.renderObjectBufferAddress };
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GlobalShaderDataPushConstant), &pcData);

        // Draw
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindIndexBuffer(commandBuffer, staticBuffers.indexBuffer.bufferHandle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirectCount(commandBuffer, staticBuffers.indirectDrawBuffer.buffer.bufferHandle, offsetof(IndirectDrawData, drawIndirect),
            staticBuffers.indirectCountBuffer.buffer.bufferHandle, 0, IndirectDrawElementCount, sizeof(IndirectDrawData));

        // End pass
        vkCmdEndRendering(commandBuffer);
    }

    static void DrawTransparents(VkCommandBuffer commandBuffer, VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorWriteCount,
        VkPipeline pipeline, VkPipelineLayout layout, VkDescriptorSet* textureSet, VkRenderingAttachmentInfo& colorAttachmentInfo,
        VkRenderingAttachmentInfo& depthAttachmentInfo, VkExtent2D drawExtent, VulkanRenderer::StaticBuffers& staticBuffers,
        uint32_t drawCount, VkInstance instance, uint8_t bRaytracing, uint32_t tlasCount, VkAccelerationStructureKHR* pTlas)
    {
        // Render pass begin
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        BeginRendering(commandBuffer, drawExtent, { 0, 0 }, 1, &colorAttachmentInfo, &depthAttachmentInfo, nullptr);

        // Raytracing
        VkWriteDescriptorSetAccelerationStructureKHR layoutAccelerationStructurePNext{};
        if (bRaytracing)
        {
            layoutAccelerationStructurePNext.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
            layoutAccelerationStructurePNext.accelerationStructureCount = tlasCount;
            layoutAccelerationStructurePNext.pAccelerationStructures = pTlas;
            //tlasBuffer.descriptorWrite.pNext = &layoutAccelerationStructurePNext;
            //PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, PushDescriptorSetID, 1, pDescriptorWrites);
        }

        // Descriptors
        PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, PushDescriptorSetID, descriptorWriteCount, pDescriptorWrites);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, TextureDescriptorSetID, 1, textureSet, 0, nullptr);

        // Push constants
        GlobalShaderDataPushConstant pcData{ staticBuffers.transparentRenderObjectBufferAddress };
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GlobalShaderDataPushConstant), &pcData);

        // Draw
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindIndexBuffer(commandBuffer, staticBuffers.indexBuffer.bufferHandle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirectCount(commandBuffer, staticBuffers.indirectDrawBuffer.buffer.bufferHandle, offsetof(IndirectDrawData, drawIndirect),
            staticBuffers.indirectCountBuffer.buffer.bufferHandle, 0, IndirectDrawElementCount, sizeof(IndirectDrawData));

        // End pass
        vkCmdEndRendering(commandBuffer);
    }

    static void DrawGeometryONPC(VkCommandBuffer commandBuffer, VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorWriteCount,
        VkPipeline pipeline, VkPipelineLayout layout, VkDescriptorSet* textureSet, VkRenderingAttachmentInfo& colorAttachmentInfo,
        VkRenderingAttachmentInfo& depthAttachmentInfo, VkExtent2D drawExtent, VulkanRenderer::StaticBuffers& staticBuffers,
        uint32_t drawCount, VkInstance instance, uint8_t bRaytracing, uint32_t tlasCount, VkAccelerationStructureKHR* pTlas, 
        BlitML::mat4* pOnpcMatrix)
    {
        // Render pass begin
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        BeginRendering(commandBuffer, drawExtent, { 0, 0 }, 1, &colorAttachmentInfo, &depthAttachmentInfo, nullptr);

        // Raytracing
        VkWriteDescriptorSetAccelerationStructureKHR layoutAccelerationStructurePNext{};
        if (bRaytracing)
        {
            layoutAccelerationStructurePNext.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
            layoutAccelerationStructurePNext.accelerationStructureCount = tlasCount;
            layoutAccelerationStructurePNext.pAccelerationStructures = pTlas;
            //tlasBuffer.descriptorWrite.pNext = &layoutAccelerationStructurePNext;
            //PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, PushDescriptorSetID, 1, pDescriptorWrites);
        }

        // Descriptors
        PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, PushDescriptorSetID, descriptorWriteCount, pDescriptorWrites);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, TextureDescriptorSetID, 1, textureSet, 0, nullptr);

        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BlitML::mat4), pOnpcMatrix);

        // Draw
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindIndexBuffer(commandBuffer, staticBuffers.indexBuffer.bufferHandle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexedIndirectCount(commandBuffer, staticBuffers.indirectDrawBuffer.buffer.bufferHandle, offsetof(IndirectDrawData, drawIndirect),
            staticBuffers.indirectCountBuffer.buffer.bufferHandle, 0, IndirectDrawElementCount, sizeof(IndirectDrawData));

        // End pass
        vkCmdEndRendering(commandBuffer);
    }

    static void GenerateDepthPyramid(VkCommandBuffer commandBuffer, PushDescriptorImage& depthAttachment, PushDescriptorImage& depthPyramid, 
        VkExtent2D depthPyramidExtent, uint32_t depthPyramidMipCount, VkImageView* depthPyramidMips, VkPipeline pipeline, VkPipelineLayout layout, 
        VkInstance instance)
    {
        VkImageMemoryBarrier2 depthTransitionBarriers[2]{};
        // Depth attachment to shader read
        ImageMemoryBarrier(depthAttachment.image.image, depthTransitionBarriers[0], VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
            VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // Depth pyramid to shader write
        ImageMemoryBarrier(depthPyramid.image.image, depthTransitionBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // Execute
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 2, depthTransitionBarriers);

        // Creates the descriptor write array. Initially it will holds the depth attachment layout and image view
        depthAttachment.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthAttachment.descriptorInfo.imageView = depthAttachment.image.imageView;
        VkWriteDescriptorSet pyramidDescriptors[2]{};
        pyramidDescriptors[0] = depthAttachment.descriptorWrite;
        pyramidDescriptors[1] = depthPyramid.descriptorWrite;

        // Binds the compute pipeline. It will be dispatched for every loop iteration
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        for(size_t i = 0; i < depthPyramidMipCount; ++i)
        {
            // Updates image info for each iteration
            if(i != 0)
            {
                depthAttachment.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                depthAttachment.descriptorInfo.imageView = depthPyramidMips[i - 1];

                pyramidDescriptors[0].pImageInfo = &depthAttachment.descriptorInfo;
            }

			// The depth pyramid image view is updated for the current mip level
            depthPyramid.descriptorInfo.imageView = depthPyramidMips[i];
            pyramidDescriptors[1].pImageInfo = &depthPyramid.descriptorInfo;

            // Descriptors
            PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, BLIT_ARRAY_SIZE(pyramidDescriptors), pyramidDescriptors);

            // Mip size calculcations
            uint32_t levelWidth = BlitML::Max(1u, (depthPyramidExtent.width) >> i);
            uint32_t levelHeight = BlitML::Max(1u, (depthPyramidExtent.height) >> i);

            // Push constant for extent
            BlitML::vec2 pyramidLevelExtentPushConstant{float(levelWidth), float(levelHeight)};
            vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlitML::vec2), &pyramidLevelExtentPushConstant);

            // Dispatch the shader to generate the current mip level of the depth pyramid
            vkCmdDispatch(commandBuffer, levelWidth / 32 + 1, levelHeight / 32 + 1, 1);

            // Barrier for the next loop, since it will use the current mip as the read descriptor
            VkImageMemoryBarrier2 dispatchWriteBarrier{};
            ImageMemoryBarrier(depthPyramid.image.image, dispatchWriteBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, 
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &dispatchWriteBarrier);
        }

        // Pipeline barrier to transition back to depth attachment optimal layout
        VkImageMemoryBarrier2 depthAttachmentReadBarrier{};
        ImageMemoryBarrier(depthAttachment.image.image, depthAttachmentReadBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &depthAttachmentReadBarrier);
    }

    static void CopyPyramidToSwapchain(VkInstance instance, VkCommandBuffer commandBuffer, PushDescriptorImage& depthPyramid, Swapchain& swapchain, VkExtent2D drawExtent,
        VkExtent2D depthPyramidExtent, uint32_t depthPyramidMipCount, VkImageView* depthPyramidMips, VkPipeline pipeline, VkPipelineLayout layout, uint32_t swapchainIdx, 
        uint32_t pyramidMip, VkSampler sampler)
    {
        // Swapchain image and attachment image descriptors
        VkWriteDescriptorSet swapchainImageWrite{};
        VkDescriptorImageInfo swapchainImageDescriptorInfo{};
        WriteImageDescriptorSets(swapchainImageWrite, swapchainImageDescriptorInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE,
            0, VK_IMAGE_LAYOUT_GENERAL, swapchain.swapchainImageViews[swapchainIdx]);

        depthPyramid.descriptorInfo.imageView = depthPyramidMips[pyramidMip];
        uint32_t levelWidth = BlitML::Max(1u, (depthPyramidExtent.width) >> pyramidMip);
        uint32_t levelHeight = BlitML::Max(1u, (depthPyramidExtent.height) >> pyramidMip);

        depthPyramid.descriptorWrite.pImageInfo = &depthPyramid.descriptorInfo;
        depthPyramid.descriptorInfo.sampler = sampler;
        depthPyramid.descriptorWrite.dstBinding = 1;
        depthPyramid.descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        VkWriteDescriptorSet colorAttachmentCopyWrite[2] =
        {
            depthPyramid.descriptorWrite, swapchainImageWrite
        };
        PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 2, colorAttachmentCopyWrite);

        // Extent push constant
        BlitML::vec2 presentImageExtentPcVal
        {
            static_cast<float>(swapchain.swapchainExtent.width), static_cast<float>(swapchain.swapchainExtent.height)
        };
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlitML::vec2), &presentImageExtentPcVal);

        // Dispatches copy shader
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdDispatch(commandBuffer, BlitML::GetComputeShaderGroupSize(levelWidth, 8), drawExtent.height / 8 + 1, 1);

        // Layout transition barrier
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(swapchain.swapchainImages[swapchainIdx], presentImageBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT,
            0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);

        depthPyramid.descriptorWrite.dstBinding = 0;
        depthPyramid.descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    }

    static void DrawBackgroundImage(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkPipelineLayout layout, 
        VkInstance instance, VkImageView view, VkExtent2D extent)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        VkWriteDescriptorSet backgroundImageWrite{};
        VkDescriptorImageInfo backgroundImageInfo{};
        WriteImageDescriptorSets(backgroundImageWrite, backgroundImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 
            VK_NULL_HANDLE, 0, VK_IMAGE_LAYOUT_GENERAL, view);

	    PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, Ce_SinglePointer, &backgroundImageWrite);

	    BackgroundShaderPushConstant pc;
	    pc.data1 = BlitML::vec4(1, 0, 0, 1);
	    pc.data2 = BlitML::vec4(0, 0, 1, 1);
	    vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BackgroundShaderPushConstant), &pc);

	    vkCmdDispatch(commandBuffer, uint32_t(std::ceil(extent.width / 16.0)), 
        uint32_t(std::ceil(extent.height / 16.0)), 1);
    }

    static void CopyColorAttachmentToSwapchainImage(VkCommandBuffer commandBuffer, VkImageView swapchainView, VkImage swapchainImage, 
        PushDescriptorImage& colorAttachment, VkExtent2D drawExtent, VkPipeline pipeline, VkPipelineLayout layout, VkInstance instance)
    {
        // Swapchain image and attachment image descriptors
        VkWriteDescriptorSet swapchainImageWrite{};
        VkDescriptorImageInfo swapchainImageDescriptorInfo{};
        WriteImageDescriptorSets(swapchainImageWrite, swapchainImageDescriptorInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, 
            0, VK_IMAGE_LAYOUT_GENERAL, swapchainView);
        VkWriteDescriptorSet colorAttachmentCopyWrite[2] = 
        {
            colorAttachment.descriptorWrite, swapchainImageWrite
        };
        PushDescriptors(instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 2, colorAttachmentCopyWrite);

        // Extent push constant
        BlitML::vec2 presentImageExtentPcVal
        {
            static_cast<float>(drawExtent.width), static_cast<float>(drawExtent.height)
        };
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BlitML::vec2), &presentImageExtentPcVal);

        // Dispatches copy shader
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdDispatch(commandBuffer, drawExtent.width / 8 + 1, drawExtent.height / 8 + 1, 1);

        // Layout transition barrier
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(swapchainImage, presentImageBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 
            0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);
    }

    static void PresentToSwapchain(VkDevice device, VkQueue queue, VkSwapchainKHR* pSwapchains, uint32_t swapchainCount,
        uint32_t waitSemaphoreCount, VkSemaphore* pWaitSemaphores, uint32_t* pImageIndices, VkResult* pResults = nullptr,
        void* pNextChain = nullptr)
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

    static void RecreateSwapchain(VkDevice device, VkPhysicalDevice pdv, VkSurfaceKHR surface, VmaAllocator vma, 
        Swapchain& swapchainData, Queue graphicQueue, Queue presentQueue, Queue computeQueue, 
        PushDescriptorImage& colorAttachment, VkRenderingAttachmentInfo& colorAttachmentInfo,
        PushDescriptorImage& depthAttachment, VkRenderingAttachmentInfo& depthAttachmentInfo,
        PushDescriptorImage& depthPyramid, uint8_t& depthPyramidMipCount, VkImageView* depthPyramidMips, VkExtent2D& depthPyramidExtent,
        uint32_t windowWidth, uint32_t windowHeight, VkExtent2D& drawExtent)
    {
        vkDeviceWaitIdle(device);

        // Creates new swapchain, after saving the old handle to destroy it
        auto oldSwapchain = swapchainData.swapchainHandle;
        CreateSwapchain(device, surface, pdv,  windowWidth, windowHeight, graphicQueue, 
            presentQueue, computeQueue, nullptr, swapchainData, oldSwapchain);
        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);

        // Destroys old depth pyramid
        depthPyramid.image.CleanupResources(vma, device);
        for(uint8_t i = 0; i < depthPyramidMipCount; ++i)
        {
            vkDestroyImageView(device, depthPyramidMips[i], nullptr);
        }

        // Destroys old attachments
        colorAttachment.image.CleanupResources(vma, device);
        depthAttachment.image.CleanupResources(vma, device);

        drawExtent.width = windowWidth;
        drawExtent.height = windowHeight;

        // Recreates color attachment
        CreatePushDescriptorImage(device, vma, colorAttachment, {drawExtent.width, drawExtent.height, 1}, 
            ce_colorAttachmentFormat, ce_colorAttachmentImageUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY);
        CreateRenderingAttachmentInfo(colorAttachmentInfo, colorAttachment.image.imageView, ce_ColorAttachmentLayout, 
            VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, ce_WindowClearColor);

        // Recreates Depth attachment
        CreatePushDescriptorImage(device, vma, depthAttachment, {drawExtent.width, drawExtent.height, 1}, 
            ce_depthAttachmentFormat, ce_depthAttachmentImageUsage, 1, VMA_MEMORY_USAGE_GPU_ONLY);
        CreateRenderingAttachmentInfo(depthAttachmentInfo, depthAttachment.image.imageView,
            ce_DepthAttachmentLayout, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
            { 0, 0, 0, 0 }, { 0, 0 });

        // Recreates the depth pyramid after the old one has been destroyed
        CreateDepthPyramid(depthPyramid, depthPyramidExtent, depthPyramidMips, depthPyramidMipCount, drawExtent, device, vma);
    }


    void VulkanRenderer::Update(const BlitzenEngine::DrawContext& context)
    {
        if (context.m_camera.transformData.bWindowResize)
        {
            RecreateSwapchain(m_device, m_physicalDevice, m_surface.handle, m_allocator,
                m_swapchainValues, m_graphicsQueue, m_presentQueue, m_computeQueue,
                m_colorAttachment, m_colorAttachmentInfo, m_depthAttachment, m_depthAttachmentInfo,
                m_depthPyramid, m_depthPyramidMipLevels, m_depthPyramidMips, m_depthPyramidExtent,
                uint32_t(context.m_camera.transformData.windowWidth), uint32_t(context.m_camera.transformData.windowHeight),
                m_drawExtent);

            context.m_camera.viewData.pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
            context.m_camera.viewData.pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);
        }

        auto& fTools = m_frameToolsList[m_currentFrame];
        auto& vBuffers = m_varBuffers[m_currentFrame];

        // The buffer info for vbuffers is updated 
        m_graphicsDescriptors[ce_viewDataWriteElement].pBufferInfo = &vBuffers.viewDataBuffer.bufferInfo;
        m_drawCullDescriptors[ce_viewDataWriteElement].pBufferInfo = &vBuffers.viewDataBuffer.bufferInfo;
        m_graphicsDescriptors[Ce_TransformBufferGraphicsDescriptorId].pBufferInfo = &vBuffers.transformBuffer.bufferInfo;
        m_drawCullDescriptors[Ce_TransformBufferDrawCullDescriptorId].pBufferInfo = &vBuffers.transformBuffer.bufferInfo;
    }

    void VulkanRenderer::UpdateObjectTransform(uint32_t transformId, BlitzenEngine::MeshTransform* pTransform)
    {
        auto pData = m_varBuffers[m_currentFrame].pTransformData;
        BlitzenCore::BlitMemCopy(pData + transformId, pTransform, sizeof(BlitzenEngine::MeshTransform));
    }

    void VulkanRenderer::DrawFrame(BlitzenEngine::DrawContext& context)
    {
        auto& fTools = m_frameToolsList[m_currentFrame];
        auto& vBuffers = m_varBuffers[m_currentFrame];

        // Waits for the fence in the current frame tools struct to be signaled and resets it for next time when it gets signalled
        vkWaitForFences(m_device, 1, &fTools.inFlightFence.handle, VK_TRUE, ce_fenceTimeout);
        VK_CHECK(vkResetFences(m_device, 1, &(fTools.inFlightFence.handle)))
        UpdateBuffers(context, fTools, vBuffers, m_transferQueue.handle);

        if (context.m_camera.transformData.bFreezeFrustum)
        {
            // Only change the matrix that moves the camera if the freeze frustum debug functionality is active
            vBuffers.viewDataBuffer.pData->projectionViewMatrix = context.m_camera.viewData.projectionViewMatrix;
        }
        else
        {
            *(vBuffers.viewDataBuffer.pData) = context.m_camera.viewData;
        }

        // Swapchain image, needed to present the color attachment results
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_swapchainValues.swapchainHandle, ce_swapchainImageTimeout, fTools.imageAcquiredSemaphore.handle, VK_NULL_HANDLE, &swapchainIdx);

        // Color attachment working layout depends on if there are any render objects
        auto colorAttachmentWorkingLayout = context.m_renders.m_renderCount ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

        if constexpr (BlitzenCore::Ce_BuildClusters)
        {
            // Fist culling pass with separate command buffer
            BeginCommandBuffer(fTools.computeCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			// Generates cluster dispatch data and count for the opaque render objects
            PreClusterDrawCull(fTools.computeCommandBuffer, m_preClusterCullPipeline.handle, m_clusterCullLayout.handle,
                BLIT_ARRAY_SIZE(m_drawCullDescriptors), m_drawCullDescriptors, m_staticBuffers.clusterCountBuffer.bufferHandle,
                m_staticBuffers.clusterCountBufferAddress, m_staticBuffers.clusterDispatchBuffer.bufferHandle,
                m_staticBuffers.clusterDispatchBufferAddress, m_staticBuffers.clusterCountCopyBuffer.bufferHandle,
                context.m_renders.m_renderCount, m_staticBuffers.renderObjectBufferAddress, Ce_InitialCulling, m_instance);

			// Generates cluster dispatch data and count for the transparent render objects
            PreClusterDrawCull(fTools.computeCommandBuffer, m_preClusterCullPipeline.handle, m_clusterCullLayout.handle,
				BLIT_ARRAY_SIZE(m_drawCullDescriptors), m_drawCullDescriptors, m_staticBuffers.transparentClusterCountBuffer.bufferHandle,
                m_staticBuffers.transparentClusterCountBufferAddress, m_staticBuffers.transparentClusterDispatchBuffer.bufferHandle,
                m_staticBuffers.transparentClusterDispatchBufferAddress, m_staticBuffers.transparentClusterCountCopyBuffer.bufferHandle,
				uint32_t(context.m_renders.m_transparentRenderCount), m_staticBuffers.transparentRenderObjectBufferAddress,
				Ce_InitialCulling, m_instance);

            // Submits command buffer to generate cluster dispatch count
            VkSemaphoreSubmitInfo bufferUpdateWaitSemaphore{};
            CreateSemahoreSubmitInfo(bufferUpdateWaitSemaphore, fTools.buffersReadySemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            VkSemaphoreSubmitInfo waitForClusterData{};
            CreateSemahoreSubmitInfo(waitForClusterData, fTools.preClusterCullingDoneSemaphore.handle,VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            SubmitCommandBuffer(m_computeQueue.handle, fTools.computeCommandBuffer, Ce_SinglePointer, 
                &bufferUpdateWaitSemaphore, Ce_SinglePointer, &waitForClusterData, fTools.preCulsterCullingFence.handle);
            vkWaitForFences(m_device, Ce_SinglePointer, &fTools.preCulsterCullingFence.handle, VK_TRUE, ce_fenceTimeout);
            vkResetFences(m_device, Ce_SinglePointer, &fTools.preCulsterCullingFence.handle);

            // Command recording begins again
            BeginCommandBuffer(fTools.commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            
            DefineViewportAndScissor(fTools.commandBuffer, m_swapchainValues.swapchainExtent);

            // Attachment barriers for layout transitions before rendering
            VkImageMemoryBarrier2 renderingAttachmentDefinitionBarriers[2] = {};
            ImageMemoryBarrier(m_colorAttachment.image.image, renderingAttachmentDefinitionBarriers[0],
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, colorAttachmentWorkingLayout, VK_IMAGE_ASPECT_COLOR_BIT,
                0, VK_REMAINING_MIP_LEVELS);
            ImageMemoryBarrier(m_depthAttachment.image.image, renderingAttachmentDefinitionBarriers[1],
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT,
                0, VK_REMAINING_MIP_LEVELS);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, renderingAttachmentDefinitionBarriers);

            auto dispatchCount
            {
                static_cast<uint32_t>(*reinterpret_cast<uint32_t*>(m_staticBuffers.clusterCountCopyBuffer.allocationInfo.pMappedData))
            };
			auto transparentDispatchCount
			{
				static_cast<uint32_t>(*reinterpret_cast<uint32_t*>(m_staticBuffers.transparentClusterCountCopyBuffer.allocationInfo.pMappedData))
			};

            // Culls opaque render object clusters
            ClusterCull(fTools.commandBuffer, m_intialClusterCullPipeline.handle, m_clusterCullLayout.handle,
                BLIT_ARRAY_SIZE(m_drawCullDescriptors), m_drawCullDescriptors, m_staticBuffers.clusterCountBuffer.bufferHandle,
                m_staticBuffers.clusterCountCopyBuffer, m_staticBuffers.clusterDispatchBuffer.bufferHandle,
                m_staticBuffers.clusterDispatchBufferAddress, m_staticBuffers.indirectCountBuffer.buffer.bufferHandle,
                m_staticBuffers.indirectDrawBuffer.buffer.bufferHandle, dispatchCount, m_staticBuffers.renderObjectBufferAddress, m_instance);

            DrawGeometry(fTools.commandBuffer, m_graphicsDescriptors, BLIT_ARRAY_SIZE(m_graphicsDescriptors),
                m_opaqueGeometryPipeline.handle, m_graphicsPipelineLayout.handle, &m_textureDescriptorSet, m_colorAttachmentInfo,
                m_depthAttachmentInfo, m_drawExtent, m_staticBuffers, dispatchCount, Ce_InitialCulling, m_instance,
                m_stats.bRayTracingSupported, Ce_SinglePointer, &m_staticBuffers.tlasData.handle);
            
            if (m_stats.bTranspartentObjectsExist)
            {
                ClusterCull(fTools.commandBuffer, m_transparentClusterCullPipeline.handle, m_clusterCullLayout.handle,
                    BLIT_ARRAY_SIZE(m_drawCullDescriptors), m_drawCullDescriptors, m_staticBuffers.transparentClusterCountBuffer.bufferHandle,
                    m_staticBuffers.transparentClusterCountCopyBuffer, m_staticBuffers.transparentClusterDispatchBuffer.bufferHandle,
                    m_staticBuffers.transparentClusterDispatchBufferAddress,m_staticBuffers.indirectCountBuffer.buffer.bufferHandle,
                    m_staticBuffers.indirectDrawBuffer.buffer.bufferHandle, transparentDispatchCount, m_staticBuffers.transparentRenderObjectBufferAddress, 
                    m_instance);

                DrawTransparents(fTools.commandBuffer, m_graphicsDescriptors, BLIT_ARRAY_SIZE(m_graphicsDescriptors),
                    m_postPassGeometryPipeline.handle, m_graphicsPipelineLayout.handle, &m_textureDescriptorSet, m_colorAttachmentInfo, m_depthAttachmentInfo,
                    m_drawExtent, m_staticBuffers, transparentDispatchCount, m_instance, m_stats.bRayTracingSupported, Ce_SinglePointer, &m_staticBuffers.tlasData.handle);
            }


            // Image barriers to transition the layout of the color attachment and the swapchain image
            VkImageMemoryBarrier2 colorAttachmentTransferBarriers[2] = {};
            ImageMemoryBarrier(m_colorAttachment.image.image, colorAttachmentTransferBarriers[0], VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, colorAttachmentWorkingLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            ImageMemoryBarrier(m_swapchainValues.swapchainImages[size_t(swapchainIdx)], colorAttachmentTransferBarriers[1], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT,
                0, VK_REMAINING_MIP_LEVELS);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, colorAttachmentTransferBarriers);

            // Copies the color attachment to the swapchain image
            CopyColorAttachmentToSwapchainImage(fTools.commandBuffer, m_swapchainValues.swapchainImageViews[swapchainIdx],
                m_swapchainValues.swapchainImages[swapchainIdx], m_colorAttachment,m_drawExtent, m_generatePresentationPipeline.handle, 
                m_generatePresentationLayout.handle, m_instance);

            VkSemaphoreSubmitInfo waitSemaphores[2]{ {}, {} };
            CreateSemahoreSubmitInfo(waitSemaphores[0], fTools.imageAcquiredSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            CreateSemahoreSubmitInfo(waitSemaphores[1], fTools.preClusterCullingDoneSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            VkSemaphoreSubmitInfo signalSemaphore{};
            CreateSemahoreSubmitInfo(signalSemaphore, fTools.readyToPresentSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
            SubmitCommandBuffer(m_graphicsQueue.handle, fTools.commandBuffer, 2, waitSemaphores, 1, &signalSemaphore, fTools.inFlightFence.handle);

            PresentToSwapchain(m_device, m_graphicsQueue.handle, &m_swapchainValues.swapchainHandle,
                1, 1, &fTools.readyToPresentSemaphore.handle, &swapchainIdx);
        }

        else
        {
            // The command buffer recording begin here (stops when submit is called)
            BeginCommandBuffer(fTools.commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            // The viewport and scissor are dynamic, so they should be set here
            DefineViewportAndScissor(fTools.commandBuffer, m_swapchainValues.swapchainExtent);
            // Attachment barriers for layout transitions before rendering
            VkImageMemoryBarrier2 renderingAttachmentDefinitionBarriers[2] = {};
            ImageMemoryBarrier(m_colorAttachment.image.image, renderingAttachmentDefinitionBarriers[0], VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, colorAttachmentWorkingLayout, 
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            ImageMemoryBarrier(m_depthAttachment.image.image, renderingAttachmentDefinitionBarriers[1],
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT,
                0, VK_REMAINING_MIP_LEVELS);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, renderingAttachmentDefinitionBarriers);

            if (context.m_renders.m_renderCount == 0)
            {
                // TODO: Change this so that it instantly goes to present and quits the function before going further
                DrawBackgroundImage(fTools.commandBuffer, m_basicBackgroundPipeline.handle, m_basicBackgroundLayout.handle,
                m_instance, m_colorAttachment.image.imageView, m_drawExtent);
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

            // First culling pass
            DrawCullFirstPass(fTools.commandBuffer, m_instance, m_initialDrawCullPipeline.handle, m_drawCullLayout.handle,
                m_staticBuffers, vBuffers, context.m_renders.m_renderCount, BLIT_ARRAY_SIZE(m_drawCullDescriptors),
                m_drawCullDescriptors, m_staticBuffers.renderObjectBufferAddress);

            // First draw pass
            DrawGeometry(fTools.commandBuffer, m_graphicsDescriptors, BLIT_ARRAY_SIZE(m_graphicsDescriptors), m_opaqueGeometryPipeline.handle, 
                m_graphicsPipelineLayout.handle, &m_textureDescriptorSet, m_colorAttachmentInfo, m_depthAttachmentInfo,
                m_drawExtent, m_staticBuffers, context.m_renders.m_renderCount, Ce_InitialCulling, m_instance, m_stats.bRayTracingSupported,
                Ce_SinglePointer, &m_staticBuffers.tlasData.handle);

            // Depth pyramid generation
            GenerateDepthPyramid(fTools.commandBuffer, m_depthAttachment, m_depthPyramid, m_depthPyramidExtent,
                m_depthPyramidMipLevels, m_depthPyramidMips, m_depthPyramidGenerationPipeline.handle,
                m_depthPyramidGenerationLayout.handle, m_instance);

            // Second culling pass 
            DrawCullOcclusionPass(fTools.commandBuffer, m_instance, m_lateDrawCullPipeline.handle, m_drawCullLayout.handle,
                m_staticBuffers, vBuffers, m_depthPyramid, m_depthAttachment, context.m_renders.m_renderCount,
                BLIT_ARRAY_SIZE(m_drawCullDescriptors), m_drawCullDescriptors, m_staticBuffers.renderObjectBufferAddress);

            // Second draw pass
            DrawGeometry(fTools.commandBuffer, m_graphicsDescriptors, BLIT_ARRAY_SIZE(m_graphicsDescriptors),
                m_opaqueGeometryPipeline.handle, m_graphicsPipelineLayout.handle, &m_textureDescriptorSet, m_colorAttachmentInfo, m_depthAttachmentInfo,
                m_drawExtent, m_staticBuffers, context.m_renders.m_renderCount, Ce_LateCulling, m_instance, m_stats.bRayTracingSupported,
                Ce_SinglePointer, &m_staticBuffers.tlasData.handle);

            if (m_stats.bObliqueNearPlaneClippingObjectsExist)
            {
                // Replace the regular render object write with the onpc one
                m_graphicsDescriptors[2] = m_staticBuffers.onpcReflectiveRenderObjectBuffer.descriptorWrite;
                m_drawCullDescriptors[1] = m_staticBuffers.onpcReflectiveRenderObjectBuffer.descriptorWrite;

                DispatchRenderObjectCullingComputeShader(fTools.commandBuffer, m_onpcDrawCullPipeline.handle, m_drawCullLayout.handle,
                    BLIT_ARRAY_SIZE(m_drawCullDescriptors), m_drawCullDescriptors, m_staticBuffers.indirectCountBuffer.buffer.bufferHandle,
                    m_staticBuffers.indirectDrawBuffer.buffer.bufferHandle, m_staticBuffers.visibilityBuffer.buffer.bufferHandle,
                    m_depthAttachment, m_depthPyramid, context.m_renders.m_onpcRenderCount, m_staticBuffers.renderObjectBufferAddress,
                    Ce_LateCulling, m_instance);

                DrawGeometryONPC(fTools.commandBuffer, m_graphicsDescriptors, BLIT_ARRAY_SIZE(m_graphicsDescriptors), m_onpcReflectiveGeometryPipeline.handle, 
                    m_onpcReflectiveGeometryLayout.handle, &m_textureDescriptorSet, m_colorAttachmentInfo, m_depthAttachmentInfo,
                    m_drawExtent, m_staticBuffers, context.m_renders.m_onpcRenderCount, m_instance,
                    m_stats.bRayTracingSupported, Ce_SinglePointer, &m_staticBuffers.tlasData.handle, &context.m_camera.onbcProjectionMatrix);
            }

            if (m_stats.bTranspartentObjectsExist)
            {
                DrawCullFirstPass(fTools.commandBuffer, m_instance, m_transparentDrawCullPipeline.handle, m_drawCullLayout.handle,
                    m_staticBuffers, vBuffers, uint32_t(context.m_renders.m_transparentRenderCount), BLIT_ARRAY_SIZE(m_drawCullDescriptors),
                    m_drawCullDescriptors, m_staticBuffers.transparentRenderObjectBufferAddress);

                DrawTransparents(fTools.commandBuffer, m_graphicsDescriptors, BLIT_ARRAY_SIZE(m_graphicsDescriptors),
                    m_postPassGeometryPipeline.handle, m_graphicsPipelineLayout.handle, &m_textureDescriptorSet, m_colorAttachmentInfo, m_depthAttachmentInfo,
                    m_drawExtent, m_staticBuffers, uint32_t(context.m_renders.m_transparentRenderCount), m_instance,
                    m_stats.bRayTracingSupported, Ce_SinglePointer, &m_staticBuffers.tlasData.handle);
            }

            /*
            Presentation:
            -The color attachment is copied to the current swapchain image
            -The commands are submitted
            -The swapchain image is presented
            */

            // Image barriers to transition the layout of the color attachment and the swapchain image
            VkImageMemoryBarrier2 colorAttachmentTransferBarriers[2] = {};
            ImageMemoryBarrier(m_colorAttachment.image.image, colorAttachmentTransferBarriers[0],
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
                colorAttachmentWorkingLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            ImageMemoryBarrier(m_swapchainValues.swapchainImages[static_cast<size_t>(swapchainIdx)],
                colorAttachmentTransferBarriers[1],
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT,
                0, VK_REMAINING_MIP_LEVELS);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, colorAttachmentTransferBarriers);

            // Copies the color attachment to the swapchain image
            if constexpr (BlitzenCore::Ce_DepthPyramidDebug)
            {
                CopyPyramidToSwapchain(m_instance, fTools.commandBuffer, m_depthPyramid, m_swapchainValues, m_drawExtent,
                    m_depthPyramidExtent, m_depthPyramidMipLevels, m_depthPyramidMips, m_generatePresentationPipeline.handle, 
                    m_generatePresentationLayout.handle, swapchainIdx, context.m_camera.transformData.debugPyramidLevel, 
                    m_depthAttachment.sampler.handle);
            }
            else
            {
                CopyColorAttachmentToSwapchainImage(fTools.commandBuffer, m_swapchainValues.swapchainImageViews[swapchainIdx],
                    m_swapchainValues.swapchainImages[swapchainIdx], m_colorAttachment, m_drawExtent, m_generatePresentationPipeline.handle,
                    m_generatePresentationLayout.handle, m_instance);
            }
            

            // Adds semaphores and submits command buffer
            VkSemaphoreSubmitInfo waitSemaphores[2]{ {}, {} };
            CreateSemahoreSubmitInfo(waitSemaphores[0], fTools.imageAcquiredSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
            CreateSemahoreSubmitInfo(waitSemaphores[1], fTools.buffersReadySemaphore.handle, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
            VkSemaphoreSubmitInfo signalSemaphore{};
            CreateSemahoreSubmitInfo(signalSemaphore, fTools.readyToPresentSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
            SubmitCommandBuffer(m_graphicsQueue.handle, fTools.commandBuffer, 2, waitSemaphores, 1, &signalSemaphore, fTools.inFlightFence.handle);

            PresentToSwapchain(m_device, m_graphicsQueue.handle, &m_swapchainValues.swapchainHandle, 1, 1, 
                &fTools.readyToPresentSemaphore.handle, &swapchainIdx);
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
        vkAcquireNextImageKHR(m_device, m_swapchainValues.swapchainHandle, ce_swapchainImageTimeout, fTools.imageAcquiredSemaphore.handle, 
            VK_NULL_HANDLE, &swapchainIdx);
        auto swapchainImage{ m_swapchainValues.swapchainImages[swapchainIdx] };
        auto swapchainImageView{ m_swapchainValues.swapchainImageViews[swapchainIdx] };

        // The command buffer recording begin here (stops when submit is called)
        BeginCommandBuffer(m_idleDrawCommandBuffer, 0);

        // The viewport and scissor are dynamic, so they should be set here
        DefineViewportAndScissor(m_idleDrawCommandBuffer, m_swapchainValues.swapchainExtent);

        // Attachment barriers for layout transitions before rendering
        VkImageMemoryBarrier2 colorAttachmentDefinitionBarrier{};
        ImageMemoryBarrier(swapchainImage, colorAttachmentDefinitionBarrier, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            colorAttachmentWorkingLayout, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(m_idleDrawCommandBuffer, 0, nullptr, 0, nullptr, 1, &colorAttachmentDefinitionBarrier);

        VkRenderingAttachmentInfo colorAttachmentInfo{};
        CreateRenderingAttachmentInfo(colorAttachmentInfo, swapchainImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE, { 0.1f, 0.2f, 0.3f, 0 });
        BeginRendering(m_idleDrawCommandBuffer, m_swapchainValues.swapchainExtent, { 0, 0 }, 1, &colorAttachmentInfo, nullptr, nullptr);

        vkCmdBindPipeline(m_idleDrawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_loadingTrianglePipeline.handle);

        m_loadingTriangleVertexColor *= cos(deltaTime);
        vkCmdPushConstants(m_idleDrawCommandBuffer, m_loadingTriangleLayout.handle, VK_SHADER_STAGE_VERTEX_BIT, 0,
            sizeof(BlitML::vec3), &m_loadingTriangleVertexColor);

        vkCmdDraw(m_idleDrawCommandBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(m_idleDrawCommandBuffer);

        // Create a barrier for the swapchain image to transition to present optimal
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(swapchainImage, presentImageBarrier,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 
            0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(m_idleDrawCommandBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);

        VkSemaphoreSubmitInfo waitSemaphores{};

        CreateSemahoreSubmitInfo(waitSemaphores, fTools.imageAcquiredSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        VkSemaphoreSubmitInfo signalSemaphore{};
        CreateSemahoreSubmitInfo(signalSemaphore, fTools.readyToPresentSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
        SubmitCommandBuffer(m_graphicsQueue.handle, m_idleDrawCommandBuffer, 1, &waitSemaphores, 1, &signalSemaphore, fTools.inFlightFence.handle);

        PresentToSwapchain(m_device, m_graphicsQueue.handle, &m_swapchainValues.swapchainHandle, 1, 1, &fTools.readyToPresentSemaphore.handle, &swapchainIdx);
    }
}