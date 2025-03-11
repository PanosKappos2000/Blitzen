#include "vulkanRenderer.h"

void DrawMeshTasks(VkInstance instance, VkCommandBuffer commandBuffer, VkBuffer drawBuffer, 
VkDeviceSize drawOffset, VkBuffer countBuffer, VkDeviceSize countOffset, uint32_t maxDrawCount, uint32_t stride) 
{
    auto func = (PFN_vkCmdDrawMeshTasksIndirectCountEXT) vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectCountEXT");
    if (func != nullptr) 
    {
        func(commandBuffer, drawBuffer, drawOffset, countBuffer, countOffset, maxDrawCount, stride);
    } 
}

// Call vkCmdPushDescriptorSetKHR extension function (This can be removed if I upgrade to Vulkan 1.4)
void PushDescriptors(VkInstance instance, VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t set, 
uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites)
{
    auto func = (PFN_vkCmdPushDescriptorSetKHR) vkGetInstanceProcAddr(instance, "vkCmdPushDescriptorSetKHR");
    if(func != nullptr)
    {
        func(commandBuffer, bindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
    }
}

// Temporary helper function until I make sure that my math library works for frustum culling
// I will keep it here until I implement a moving frustum
glm::vec4 glm_NormalizePlane(glm::vec4& plane)
{
    return plane / glm::length(glm::vec3(plane));
}

namespace BlitzenVulkan
{

    constexpr uint32_t ce_pushDescriptorSetID = 0;

    void VulkanRenderer::DrawFrame(BlitzenEngine::DrawContext& context)
    {
        BlitzenEngine::Camera* pCamera = context.pCamera;

        if(pCamera->transformData.windowResize)
            RecreateSwapchain(
                uint32_t(pCamera->transformData.windowWidth), 
                uint32_t(pCamera->transformData.windowHeight)
            );

        // Gets a ref to the frame tools of the current frame
        FrameTools& fTools = m_frameToolsList[m_currentFrame];
        VarBuffers& vBuffers = m_varBuffers[m_currentFrame];

        // Handle the pyramid width if the window resized
        if(pCamera->transformData.windowResize)
        {
            pCamera->viewData.pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
            pCamera->viewData.pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);
        }

        // Specifies the descriptor writes that are not static again
        pushDescriptorWritesGraphics[0] = vBuffers.viewDataBuffer.descriptorWrite;
        pushDescriptorWritesCompute[0] = vBuffers.viewDataBuffer.descriptorWrite;

        // Update the buffer info in the transform buffer descriptor write
		pushDescriptorWritesGraphics[3].pBufferInfo = &vBuffers.transformBuffer.bufferInfo;
		pushDescriptorWritesCompute[2].pBufferInfo = &vBuffers.transformBuffer.bufferInfo;

        if (context.bOnpc)
        {
            m_pushDescriptorWritesOnpcCompute[0] = vBuffers.viewDataBuffer.descriptorWrite;
			m_pushDescriptorWritesOnpcGraphics[0] = vBuffers.viewDataBuffer.descriptorWrite;
        }
        
        // Waits for the fence in the current frame tools struct to be signaled and resets it for next time when it gets signalled
        vkWaitForFences(m_device, 1, &fTools.inFlightFence.handle, VK_TRUE, 1000000000);
        VK_CHECK(vkResetFences(m_device, 1, &(fTools.inFlightFence.handle)))

		UpdateBuffers(context.pResources, fTools, vBuffers);

        vkWaitForFences(m_device, 1, &fTools.inFlightFence.handle, VK_TRUE, UINT64_MAX);
        VK_CHECK(vkResetFences(m_device, 1, &(fTools.inFlightFence.handle)))

        if(pCamera->transformData.freezeFrustum)
            // Only change the matrix that moves the camera if the freeze frustum debug functionality is active
            vBuffers.viewDataBuffer.pData->projectionViewMatrix = pCamera->viewData.projectionViewMatrix;
        else
            *(vBuffers.viewDataBuffer.pData) = pCamera->viewData;
        
        // Swapchain image, needed to present the color attachment results
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_swapchainValues.swapchainHandle, 
        1000000000, fTools.imageAcquiredSemaphore.handle, VK_NULL_HANDLE, &swapchainIdx);

        // The command buffer recording begin here (stops when submit is called)
        BeginCommandBuffer(fTools.commandBuffer, 0);

        // The viewport and scissor are dynamic, so they should be set here
        DefineViewportAndScissor(fTools.commandBuffer, m_swapchainValues.swapchainExtent);

        // Color attachment working layout depends on if there are any render objects
        VkImageLayout colorAttachmentWorkingLayout = context.pResources->renderObjectCount ? 
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
        // Attachment barriers for layout transitions before rendering
        VkImageMemoryBarrier2 renderingAttachmentDefinitionBarriers[2] = {};
        ImageMemoryBarrier(m_colorAttachment.image.image, renderingAttachmentDefinitionBarriers[0], 
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE, 
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 
            VK_IMAGE_LAYOUT_UNDEFINED, colorAttachmentWorkingLayout, VK_IMAGE_ASPECT_COLOR_BIT, 
            0, VK_REMAINING_MIP_LEVELS
        );
        ImageMemoryBarrier(m_depthAttachment.image.image, renderingAttachmentDefinitionBarriers[1], 
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE, 
            VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 
            0, VK_REMAINING_MIP_LEVELS
        );
        PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, renderingAttachmentDefinitionBarriers);



        // In case there is nothing to draw, it present a gradient colored window
        if(context.pResources->renderObjectCount == 0)
        {
	        vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, 
            m_basicBackgroundPipeline.handle);
            VkWriteDescriptorSet backgroundImageWrite{};
            VkDescriptorImageInfo backgroundImageInfo{};
            WriteImageDescriptorSets(backgroundImageWrite, backgroundImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 
            VK_NULL_HANDLE, 0, VK_IMAGE_LAYOUT_GENERAL, m_colorAttachment.image.imageView);

	        PushDescriptors(m_instance, 
            fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_basicBackgroundLayout.handle, 
            0, 1, &backgroundImageWrite);

	        BackgroundShaderPushConstant pc;
	        pc.data1 = BlitML::vec4(1, 0, 0, 1);
	        pc.data2 = BlitML::vec4(0, 0, 1, 1);
	        vkCmdPushConstants(fTools.commandBuffer, m_basicBackgroundLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, 
            sizeof(BackgroundShaderPushConstant), &pc);

	        vkCmdDispatch(fTools.commandBuffer, uint32_t(std::ceil(m_drawExtent.width / 16.0)), 
            uint32_t(std::ceil(m_drawExtent.height / 16.0)), 1);
        }
        else
        {
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
            DispatchRenderObjectCullingComputeShader(
                fTools.commandBuffer, m_initialDrawCullPipeline.handle, 
                BLIT_ARRAY_SIZE(pushDescriptorWritesCompute), pushDescriptorWritesCompute, 
                context.pResources->renderObjectCount, 
                0, 0 // Post pass and late culling boolean values
            );

            // First draw pass
            DrawGeometry(
                fTools.commandBuffer, 
                pushDescriptorWritesGraphics.Data(), uint32_t(pushDescriptorWritesGraphics.Size()), 
                m_opaqueGeometryPipeline.handle, m_graphicsPipelineLayout.handle, 
                context.pResources->renderObjectCount, 
                0 // late pass boolean value
            );

            // Depth pyramid generation
            GenerateDepthPyramid(fTools.commandBuffer);

            // Second culling pass 
            DispatchRenderObjectCullingComputeShader(
                fTools.commandBuffer, m_lateDrawCullPipeline.handle, 
                BLIT_ARRAY_SIZE(pushDescriptorWritesCompute), pushDescriptorWritesCompute, 
                context.pResources->renderObjectCount, 
                1, 0 // Late culling and post pass boolean values
            );

            // Second draw pass
            DrawGeometry(
                fTools.commandBuffer, 
                pushDescriptorWritesGraphics.Data(), uint32_t(pushDescriptorWritesGraphics.Size()), 
                m_opaqueGeometryPipeline.handle, m_graphicsPipelineLayout.handle,
                context.pResources->renderObjectCount, 
                1 // late pass boolean value
                );

            // Dispatches one more culling pass for transparent objects (this is not ideal and a better solution should be found)
            DispatchRenderObjectCullingComputeShader(
                fTools.commandBuffer, m_lateDrawCullPipeline.handle, 
                BLIT_ARRAY_SIZE(pushDescriptorWritesCompute), pushDescriptorWritesCompute, 
                context.pResources->renderObjectCount, 
                1, 1 // late culling and post pass boolean values
            );

            // Draws the transparent objects
            DrawGeometry(fTools.commandBuffer, 
                pushDescriptorWritesGraphics.Data(), uint32_t(pushDescriptorWritesGraphics.Size()), 
                m_postPassGeometryPipeline.handle, m_graphicsPipelineLayout.handle, 
                context.pResources->renderObjectCount, 
                1 // late pass boolean
            );

            if(context.bOnpc)
            {
                DispatchRenderObjectCullingComputeShader(fTools.commandBuffer, m_onpcDrawCullPipeline.handle, 
                    BLIT_ARRAY_SIZE(m_pushDescriptorWritesOnpcCompute), m_pushDescriptorWritesOnpcCompute,
                    context.pResources->onpcReflectiveRenderObjectCount, 
                    0, 0 // late culling and post pass boolean values
                );

                DrawGeometry(fTools.commandBuffer, m_pushDescriptorWritesOnpcGraphics.Data(), 
                    uint32_t(m_pushDescriptorWritesOnpcGraphics.Size()),
                    m_onpcReflectiveGeometryPipeline.handle, m_onpcReflectiveGeometryLayout.handle,
                    context.pResources->onpcReflectiveRenderObjectCount,
                    1, // late pass boolean
                    1, &pCamera->onbcProjectionMatrix // Oblique Near Plane clipping data
                );
            }
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
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS
        );
        ImageMemoryBarrier(m_swapchainValues.swapchainImages[static_cast<size_t>(swapchainIdx)], 
            colorAttachmentTransferBarriers[1], 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_NONE, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, 
            0, VK_REMAINING_MIP_LEVELS
        );
        PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, colorAttachmentTransferBarriers);

        VkWriteDescriptorSet swapchainImageWrite{};
        VkDescriptorImageInfo swapchainImageDescriptorInfo{};
        WriteImageDescriptorSets(swapchainImageWrite, swapchainImageDescriptorInfo, 
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, 0, VK_IMAGE_LAYOUT_GENERAL, 
            m_swapchainValues.swapchainImageViews[swapchainIdx]
        );
        VkWriteDescriptorSet colorAttachmentCopyWrite[2] = 
        {
            m_colorAttachment.descriptorWrite, 
            swapchainImageWrite
        };
        PushDescriptors(m_instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, 
            m_generatePresentationLayout.handle, 0, 2, colorAttachmentCopyWrite
        );
        BlitML::vec2 presentImageExtentPcVal{
            static_cast<float>(m_drawExtent.width), 
            static_cast<float>(m_drawExtent.height)
        };
        vkCmdPushConstants(fTools.commandBuffer, m_generatePresentationLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 
            0, sizeof(BlitML::vec2), &presentImageExtentPcVal // push constant offset, size and data
        );
        vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_generatePresentationPipeline.handle);
        vkCmdDispatch(fTools.commandBuffer, m_drawExtent.width / 8 + 1, m_drawExtent.height / 8 + 1, 1);

        // Create a barrier for the swapchain image to transition to present optimal
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(m_swapchainValues.swapchainImages[static_cast<size_t>(swapchainIdx)], 
            presentImageBarrier, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, 
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // layout transition
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS
        );
        PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);

        //VkSemaphoreSubmitInfo waitSemaphores[2]{ {}, {} };
        VkSemaphoreSubmitInfo waitSemaphores{};
		CreateSemahoreSubmitInfo(waitSemaphores, fTools.imageAcquiredSemaphore.handle, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
        );
		/*CreateSemahoreSubmitInfo(waitSemaphores[1], fTools.buffersReadySemaphore.handle,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
        );*/
        VkSemaphoreSubmitInfo signalSemaphore{};
		CreateSemahoreSubmitInfo(signalSemaphore, fTools.readyToPresentSemaphore.handle,
            VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
        );
        SubmitCommandBuffer(m_graphicsQueue.handle, fTools.commandBuffer, 
            1, &waitSemaphores, // waits for image to be acquired
            1, &signalSemaphore, // signals the ready to present semaphore when done
            fTools.inFlightFence.handle // next frame waits for commands to be done
        );

        PresentToSwapchain(m_device, m_graphicsQueue.handle, 
            &m_swapchainValues.swapchainHandle, 1, 
            1, &fTools.readyToPresentSemaphore.handle, // waits for this semaphore
            &swapchainIdx
        );

        m_currentFrame = (m_currentFrame + 1) % ce_framesInFlight;
    }

    void VulkanRenderer::DispatchRenderObjectCullingComputeShader(VkCommandBuffer commandBuffer, 
    VkPipeline pipeline,
    uint32_t descriptorWriteCount, VkWriteDescriptorSet* pDescriptorWrites, 
    uint32_t drawCount, 
    uint8_t lateCulling /*=0*/, uint8_t postPass /*=0*/)
    {
        // If this is after the first render pass, the shader will also need the depth pyramid image sampler to do occlusion culling
        if(lateCulling)
        {
            VkWriteDescriptorSet depthPyramidWrite{};
            VkDescriptorImageInfo depthPyramidImageInfo{};
            WriteImageDescriptorSets(depthPyramidWrite, depthPyramidImageInfo, 
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE,
                3, // Depth pyramid image binding ID
                VK_IMAGE_LAYOUT_GENERAL, m_depthPyramid.image.imageView, m_depthAttachment.sampler.handle
            );

            pDescriptorWrites[descriptorWriteCount - 1] = depthPyramidWrite;
        }

        // This pipeline barrier waits for previous indirect draw to be done with indirect count buffer,
        // before zeroing it with vkCmdFillBuffer
        VkBufferMemoryBarrier2 waitBeforeZeroingCountBuffer{};
        BufferMemoryBarrier(m_currentStaticBuffers.indirectCountBuffer.buffer.bufferHandle, 
            waitBeforeZeroingCountBuffer,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_ACCESS_2_TRANSFER_WRITE_BIT,
             0, VK_WHOLE_SIZE
        );
        PipelineBarrier(commandBuffer, 0, nullptr, 1, &waitBeforeZeroingCountBuffer, 0, nullptr);

        // Indirect count starts from 0
        vkCmdFillBuffer(commandBuffer, m_currentStaticBuffers.indirectCountBuffer.buffer.bufferHandle, 
            0, sizeof(uint32_t), 0 // offset, size of buffer and value to set
        );

        // These pipeline barriers wait for previous actions on buffers to finish, 
        // before the culling shader reads from them or writes to them.
        // The affected buffers are the visibility, indirect count and indirect draw buffers
        VkBufferMemoryBarrier2 waitBeforeDispatchingShaders[3] = {};
        // Indirect count buffer waits for vkCmdFillBuffer to finish
        BufferMemoryBarrier(m_currentStaticBuffers.indirectCountBuffer.buffer.bufferHandle, 
            waitBeforeDispatchingShaders[0], 
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 
            0, VK_WHOLE_SIZE
        );
        // The indirect draw buffer waits for the indirect draw stage to finish reading previous commands
        BufferMemoryBarrier(m_currentStaticBuffers.indirectDrawBuffer.buffer.bufferHandle, 
            waitBeforeDispatchingShaders[1], 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, 
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            0, VK_WHOLE_SIZE
        );
        // The visibility buffer waits for the previous compute shader to complete read and write operations
        // TODO: this barrier could be more specific depending on if I am in late or early stage
        BufferMemoryBarrier(m_currentStaticBuffers.visibilityBuffer.buffer.bufferHandle, 
        waitBeforeDispatchingShaders[2], 
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, 
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, 
        0, VK_WHOLE_SIZE);

        // The late culling shader needs to wait for the 3 barriers above but it also needs to wait for the depth pyramid to be generated
        if(lateCulling)
        {
            // This barrier waits for the depth generation compute shader to finish writing to the depth pyramid image
            VkImageMemoryBarrier2 waitForDepthPyramidGeneration{};
            ImageMemoryBarrier(m_depthPyramid.image.image, waitForDepthPyramidGeneration, 
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, // No layout transtions
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS
            );
            // Pipeline barrier execution
            PipelineBarrier(commandBuffer, 0, nullptr, 3, waitBeforeDispatchingShaders, 1, &waitForDepthPyramidGeneration);
        }
        // If this is the initial culling stage, simply adds the 2 barriers
        else
        {
            PipelineBarrier(commandBuffer, 0, nullptr, 3, waitBeforeDispatchingShaders, 0, nullptr);
        }

        // Set up before dispatch
        PushDescriptors(m_instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, 
            m_drawCullLayout.handle, ce_pushDescriptorSetID,
            lateCulling ? descriptorWriteCount : descriptorWriteCount - 1, // late culling has an extra binding
            pDescriptorWrites
        );
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        DrawCullShaderPushConstant pc{drawCount, postPass}; // Push constant data
        vkCmdPushConstants(commandBuffer, m_drawCullLayout.handle, VK_SHADER_STAGE_COMPUTE_BIT, 
            0,sizeof(DrawCullShaderPushConstant), &pc // push constant offset, size and data
        );

        // Dispatches the shader
        vkCmdDispatch(commandBuffer, (drawCount / 64) + 1, 1, 1);

        // Once the shader is done, a barrier needs to make sure that the graphics pipeline waits
        VkBufferMemoryBarrier2 waitForCullingShader[3] = {};
        // Stop the indirect count buffer from being read before the culling shader is done with it
        BufferMemoryBarrier(m_currentStaticBuffers.indirectCountBuffer.buffer.bufferHandle, 
            waitForCullingShader[0], 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, 
            0, VK_WHOLE_SIZE
        );
        // Stops the vertex shader from reading draw IDs and the indirect stage from reading commands
        BufferMemoryBarrier(m_currentStaticBuffers.indirectDrawBuffer.buffer.bufferHandle, 
            waitForCullingShader[1], 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, 
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT, 
            0, VK_WHOLE_SIZE
        );
        // This one might be obsolete
        BufferMemoryBarrier(m_currentStaticBuffers.visibilityBuffer.buffer.bufferHandle, 
            waitForCullingShader[2], 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
            VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
            VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, 
            0, VK_WHOLE_SIZE
        );
        // Executes the barriers
        PipelineBarrier(commandBuffer, 0, nullptr, BLIT_ARRAY_SIZE(waitForCullingShader), waitForCullingShader, 0, nullptr);
    }

    void VulkanRenderer::DrawGeometry(VkCommandBuffer commandBuffer, 
    VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorWriteCount,
    VkPipeline pipeline, VkPipelineLayout layout, uint32_t drawCount, 
    uint8_t latePass /*=0*/, uint8_t onpcPass /*=0*/, BlitML::mat4* pOnpcMatrix /*=nullptr*/)
    {
        // Creates info for the color attachment 
        VkRenderingAttachmentInfo colorAttachmentInfo{};
        CreateRenderingAttachmentInfo(colorAttachmentInfo, m_colorAttachment.image.imageView, 
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
            latePass ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR, 
            VK_ATTACHMENT_STORE_OP_STORE, 
            {0.1f, 0.2f, 0.3f, 0} // Window color
        );
        // Creates info for the depth attachment
        VkRenderingAttachmentInfo depthAttachmentInfo{};
        CreateRenderingAttachmentInfo(depthAttachmentInfo, m_depthAttachment.image.imageView, 
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 
            latePass ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR, // early pass uses clear value, late pass stores
            VK_ATTACHMENT_STORE_OP_STORE, {0, 0, 0, 0}, // Clear color (irrelevant here)
            {0, 0} // Clear depth
        );
        // Render pass begins
        BeginRendering(commandBuffer, m_drawExtent, {0, 0}, // Render area offset
            1, &colorAttachmentInfo, // Color attachment count and color attachment(s)
            &depthAttachmentInfo, nullptr // Stencil attachment
        );

        // The acceleration structure write needs this struct on its pNext chain
        VkWriteDescriptorSetAccelerationStructureKHR layoutAccelerationStructurePNext{};
        if(ce_bRaytracing)
        {
            layoutAccelerationStructurePNext.sType = 
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
            layoutAccelerationStructurePNext.accelerationStructureCount = 1;
            layoutAccelerationStructurePNext.pAccelerationStructures = 
                &m_currentStaticBuffers.tlasData.handle;
            pDescriptorWrites[7].pNext = &layoutAccelerationStructurePNext;
        }
        // Pushes the descriptors that use the push descriptor extension
        PushDescriptors(m_instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            layout, ce_pushDescriptorSetID,  
            m_stats.bRayTracingSupported ? descriptorWriteCount // Raytracing mode needs to bind the tlas buffer
            : descriptorWriteCount - 1, // tlas buffer not needed otherwise
            pDescriptorWrites
        );
        // Descriptor set for bindless textures
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            layout, 1, 1, // First set ID and set count
            &m_textureDescriptorSet, 0, nullptr // Dynamic offsets
        );
        // For oblique near-plane clipping render pass, the push constant that holds the modified matrix is needed
        if(onpcPass)
            vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 
                0, sizeof(BlitML::mat4), pOnpcMatrix
            );

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        // Mesh shader
        if(m_stats.meshShaderSupport)
        {
            DrawMeshTasks(m_instance, commandBuffer, 
                m_currentStaticBuffers.indirectTaskBuffer.buffer.bufferHandle, // Task buffer
                offsetof(IndirectTaskData, drawIndirectTasks), 
                m_currentStaticBuffers.indirectCountBuffer.buffer.bufferHandle, // Task count buffer 
                0, drawCount, sizeof(IndirectTaskData)
            );
        }
        // Vertex shader
        else
        {
            vkCmdBindIndexBuffer(commandBuffer, m_currentStaticBuffers.indexBuffer.bufferHandle, 
                0, VK_INDEX_TYPE_UINT32 // Index buffer offset and data type
            );
            vkCmdDrawIndexedIndirectCount(commandBuffer, 
                m_currentStaticBuffers.indirectDrawBuffer.buffer.bufferHandle, 
                offsetof(IndirectDrawData, drawIndirect), // indirect draw buffer offset
                m_currentStaticBuffers.indirectCountBuffer.buffer.bufferHandle,
                0, drawCount, sizeof(IndirectDrawData) // buffer offset, max draw count and stride
            );
        }

        vkCmdEndRendering(commandBuffer);
    }

    void VulkanRenderer::GenerateDepthPyramid(VkCommandBuffer commandBuffer)
    {
        VkImageMemoryBarrier2 depthTransitionBarriers[2] = {};
        // Transitions the depth attachment's layout to be read by the shader
        ImageMemoryBarrier(m_depthAttachment.image.image, depthTransitionBarriers[0], 
            VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
            VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS
        );
        // Transitions the depth pyramid image layout to be written to by the shaders
        ImageMemoryBarrier(m_depthPyramid.image.image, depthTransitionBarriers[1], 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS
        );
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 2, depthTransitionBarriers);

        // Creates the descriptor write array. Initially it will holds the depth attachment layout and image view
        m_depthAttachment.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        m_depthAttachment.descriptorInfo.imageView = m_depthAttachment.image.imageView;
        VkWriteDescriptorSet srcAndDstDepthImageDescriptors[2] = 
        {
			m_depthAttachment.descriptorWrite, m_depthPyramid.descriptorWrite
        };
        srcAndDstDepthImageDescriptors[0] = m_depthAttachment.descriptorWrite;

        // Binds the compute pipeline. It will be dispatched for every loop iteration
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_depthPyramidGenerationPipeline.handle);
        for(size_t i = 0; i < m_depthPyramidMipLevels; ++i)
        {
            // Since later iterations do not use the depth attachment image, the layout and image view need to be updated
            if(i != 0)
            {
                m_depthAttachment.descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                m_depthAttachment.descriptorInfo.imageView = m_depthPyramidMips[i - 1];

                srcAndDstDepthImageDescriptors[0].pImageInfo = &m_depthAttachment.descriptorInfo;
            }

			// The depth pyramid image view is updated for the current mip level
            m_depthPyramid.descriptorInfo.imageView = m_depthPyramidMips[i];
			srcAndDstDepthImageDescriptors[1].pImageInfo = &m_depthPyramid.descriptorInfo;

            // Push the descriptor sets
            PushDescriptors(m_instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                m_depthPyramidGenerationLayout.handle, 0, 2, srcAndDstDepthImageDescriptors
            );

            // Calculate the extent of the current depth pyramid mip level
            uint32_t levelWidth = BlitML::Max(1u, (m_depthPyramidExtent.width) >> i);
            uint32_t levelHeight = BlitML::Max(1u, (m_depthPyramidExtent.height) >> i);
            // Pass the extent to the push constant
            BlitML::vec2 pyramidLevelExtentPushConstant{
                static_cast<float>(levelWidth), 
                static_cast<float>(levelHeight)
            };
            vkCmdPushConstants(commandBuffer, m_depthPyramidGenerationLayout.handle, 
                VK_SHADER_STAGE_COMPUTE_BIT, 0, // push constant offset
                sizeof(BlitML::vec2), &pyramidLevelExtentPushConstant
            );

            // Dispatch the shader to generate the current mip level of the depth pyramid
            vkCmdDispatch(commandBuffer, levelWidth / 32 + 1, levelHeight / 32 + 1, 1);

            // Barrier for the next loop, since it will use the current mip as the read descriptor
            VkImageMemoryBarrier2 dispatchWriteBarrier{};
            ImageMemoryBarrier(m_depthPyramid.image.image, dispatchWriteBarrier, 
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, 
                VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS
            );
            PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &dispatchWriteBarrier);
        }

        // Pipeline barrier to transition back to depth attachment optimal layout
        VkImageMemoryBarrier2 depthAttachmentReadBarrier{};
        ImageMemoryBarrier(m_depthAttachment.image.image, depthAttachmentReadBarrier, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
            | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
            VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS
        );
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &depthAttachmentReadBarrier);
    }

    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags)
    {
        vkResetCommandBuffer(commandBuffer, 0);
        VkCommandBufferBeginInfo commandBufferInfo{};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferInfo.pNext = nullptr;
        commandBufferInfo.pInheritanceInfo = nullptr;
        commandBufferInfo.flags = usageFlags;
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferInfo));
    }

    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, 
        uint32_t waitSemaphoreCount /* =0 */, VkSemaphoreSubmitInfo* waitSemaphore /* =nullptr */, 
        uint32_t signalSemaphoreCount /* =0 */, VkSemaphoreSubmitInfo* signalSemaphore /* =nullptr */, 
        VkFence fence /* =VK_NULL_HANDLE */
    )
    {
        vkEndCommandBuffer(commandBuffer);

        VkCommandBufferSubmitInfo commandBufferInfo{};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferInfo.commandBuffer = commandBuffer;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &commandBufferInfo;

        submitInfo.waitSemaphoreInfoCount = waitSemaphoreCount;
        submitInfo.pWaitSemaphoreInfos = waitSemaphore;

        submitInfo.signalSemaphoreInfoCount = signalSemaphoreCount;
        submitInfo.pSignalSemaphoreInfos = signalSemaphore;

        VK_CHECK(vkQueueSubmit2(queue, 1, &submitInfo, fence))
    }

    void CreateSemahoreSubmitInfo(VkSemaphoreSubmitInfo& semaphoreInfo,
        VkSemaphore semaphore, VkPipelineStageFlags2 stage)
    {
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		semaphoreInfo.pNext = nullptr;
		semaphoreInfo.semaphore = semaphore;
		semaphoreInfo.stageMask = stage;
    }

    void CreateRenderingAttachmentInfo(VkRenderingAttachmentInfo& attachmentInfo, VkImageView imageView, VkImageLayout imageLayout, 
    VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearColorValue clearValueColor, VkClearDepthStencilValue clearValueDepth)
    {
        attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachmentInfo.pNext = nullptr;
        attachmentInfo.imageView = imageView;
        attachmentInfo.imageLayout = imageLayout;
        attachmentInfo.loadOp = loadOp;
        attachmentInfo.storeOp = storeOp;
        attachmentInfo.clearValue.color = clearValueColor;
        attachmentInfo.clearValue.depthStencil = clearValueDepth;
    }

    void BeginRendering(VkCommandBuffer commandBuffer, VkExtent2D renderAreaExtent, VkOffset2D renderAreaOffset, 
    uint32_t colorAttachmentCount, VkRenderingAttachmentInfo* pColorAttachments, VkRenderingAttachmentInfo* pDepthAttachment, 
    VkRenderingAttachmentInfo* pStencilAttachment, uint32_t viewMask /* =0 */, uint32_t layerCount /* =1 */)
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

    void DefineViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent)
    {
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = static_cast<float>(extent.height); // Start from full height (flips y axis)
        viewport.width = static_cast<float>(extent.width);
        viewport.height = -static_cast<float>(extent.height);// Move a negative amount of full height (flips y axis)
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

    void PresentToSwapchain(VkDevice device, VkQueue queue, 
        VkSwapchainKHR* pSwapchains, uint32_t swapchainCount,
        uint32_t waitSemaphoreCount, VkSemaphore* pWaitSemaphores,
        uint32_t* pImageIndices, VkResult* pResults /*=nullptr*/,
        void* pNextChain /*=nullptr*/
    )
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

    void VulkanRenderer::RecreateSwapchain(uint32_t windowWidth, uint32_t windowHeight)
    {
        vkDeviceWaitIdle(m_device);

        // Creates new swapchain, after saving the old handle to destroy it
        VkSwapchainKHR oldSwapchain = m_swapchainValues.swapchainHandle;
        CreateSwapchain(m_device, m_surface.handle, m_physicalDevice,  windowWidth, windowHeight, m_graphicsQueue, 
        m_presentQueue, m_computeQueue, m_pCustomAllocator, m_swapchainValues, oldSwapchain);

        // Destroys old swapchain
        vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);

        // Destroys old depth pyramid
        m_depthPyramid.image.CleanupResources(m_allocator, m_device);
        for(uint8_t i = 0; i < m_depthPyramidMipLevels; ++i)
        {
            vkDestroyImageView(m_device, m_depthPyramidMips[size_t(i)], m_pCustomAllocator);
        }

        // Destroy old attachments
        m_colorAttachment.image.CleanupResources(m_allocator, m_device);
        m_depthAttachment.image.CleanupResources(m_allocator, m_device);

        // Recreates the attachments
        m_drawExtent.width = windowWidth;
        m_drawExtent.height = windowHeight;
        CreatePushDescriptorImage(m_device, m_allocator, m_colorAttachment, 
            {m_drawExtent.width, m_drawExtent.height, 1}, 
            ce_colorAttachmentFormat, ce_colorAttachmentImageUsage, 
            1, VMA_MEMORY_USAGE_GPU_ONLY
        );
        CreatePushDescriptorImage(m_device, m_allocator, m_depthAttachment, 
            {m_drawExtent.width, m_drawExtent.height, 1}, 
            ce_depthAttachmentFormat, ce_depthAttachmentImageUsage, 
            1, VMA_MEMORY_USAGE_GPU_ONLY
        );

        // Recreates the depth pyramid after the old one has been destroyed
        CreateDepthPyramid(m_depthPyramid, m_depthPyramidExtent, 
        m_depthPyramidMips, m_depthPyramidMipLevels, 
        m_drawExtent, m_device, m_allocator);
    }

    void VulkanRenderer::SetupForSwitch(uint32_t windowWidth, uint32_t windowHeight)
    {
        RecreateSwapchain(windowWidth, windowHeight);
    }
}