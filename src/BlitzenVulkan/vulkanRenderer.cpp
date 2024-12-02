#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
    void VulkanRenderer::UploadDataToGPUAndSetupForRendering()
    {
        FrameToolsInit();

        CreateImage(m_device, m_allocator, m_colorAttachment, {m_drawExtent.width, m_drawExtent.height, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        CreateImage(m_device, m_allocator, m_depthAttachment, {m_drawExtent.width, m_drawExtent.height, 1}, VK_FORMAT_D32_SFLOAT, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

        /* Main opaque object graphics pipeline */
        {
            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.flags = 0;
            pipelineInfo.renderPass = VK_NULL_HANDLE; // Using dynamic rendering

            VkPipelineRenderingCreateInfo dynamicRenderingInfo{};
            dynamicRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
            VkFormat colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
            dynamicRenderingInfo.colorAttachmentCount = 1;
            dynamicRenderingInfo.pColorAttachmentFormats = &colorAttachmentFormat;
            dynamicRenderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
            pipelineInfo.pNext = &dynamicRenderingInfo;

            VkShaderModule vertexShaderModule;
            VkPipelineShaderStageCreateInfo shaderStages[2] = {};
            CreateShaderProgram(m_device, "VulkanShaders/MainObjectShader.vert.glsl.spv", VK_SHADER_STAGE_VERTEX_BIT, "main", vertexShaderModule, 
            shaderStages[0], nullptr);
            VkShaderModule fragShaderModule;
            CreateShaderProgram(m_device, "VulkanShaders/MainObjectShader.frag.glsl.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragShaderModule, 
            shaderStages[1], nullptr);
            pipelineInfo.stageCount = 2; // Hardcode for default pipeline since I know what I want
            pipelineInfo.pStages = shaderStages;

            VkPipelineInputAssemblyStateCreateInfo inputAssembly = SetTriangleListInputAssembly();
            pipelineInfo.pInputAssemblyState = &inputAssembly;

            VkDynamicState dynamicStates[2];
            VkPipelineViewportStateCreateInfo viewport{};
            VkPipelineDynamicStateCreateInfo dynamicState{};
            SetDynamicStateViewport(dynamicStates, viewport, dynamicState);
            pipelineInfo.pViewportState = &viewport;
            pipelineInfo.pDynamicState = &dynamicState;

            VkPipelineRasterizationStateCreateInfo rasterization{};
            SetRasterizationState(rasterization, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            pipelineInfo.pRasterizationState = &rasterization;

            VkPipelineMultisampleStateCreateInfo multisampling{};
            SetupMulitsampling(multisampling, VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.f, nullptr, VK_FALSE, VK_FALSE);
            pipelineInfo.pMultisampleState = &multisampling;

            VkPipelineDepthStencilStateCreateInfo depthState{};
            SetupDepthTest(depthState, VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_TRUE, VK_FALSE, 0.f, 1.f, VK_FALSE, 
            nullptr, nullptr);
            pipelineInfo.pDepthStencilState = &depthState;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            CreateColorBlendAttachment(colorBlendAttachment, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_CONSTANT_ALPHA, 
            VK_BLEND_FACTOR_CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA);
            VkPipelineColorBlendStateCreateInfo colorBlendState{};
            CreateColorBlendState(colorBlendState, 1, &colorBlendAttachment, VK_FALSE, VK_LOGIC_OP_AND);
            pipelineInfo.pColorBlendState = &colorBlendState;

            VkDescriptorSetLayoutBinding shaderDataLayoutBinding{};
            CreateDescriptorSetLayoutBinding(shaderDataLayoutBinding, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
            m_globalShaderDataLayout = CreateDescriptorSetLayout(m_device, 1, &shaderDataLayoutBinding);
            CreatePipelineLayout(m_device, &m_opaqueGraphicsPipelineLayout, 1, &m_globalShaderDataLayout, 0, nullptr);
            pipelineInfo.layout = m_opaqueGraphicsPipelineLayout;

            VkPipelineVertexInputStateCreateInfo vertexInput{};
            vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            pipelineInfo.pVertexInputState = &vertexInput;

            VK_CHECK(vkCreateGraphicsPipelines(m_device, nullptr, 1, &pipelineInfo, m_pCustomAllocator, &m_opaqueGraphicsPipeline))

            vkDestroyShaderModule(m_device, vertexShaderModule, m_pCustomAllocator);
            vkDestroyShaderModule(m_device, fragShaderModule, m_pCustomAllocator);
        }

        BlitCL::DynamicArray<BlitML::Vertex> vertices(1);
        BlitCL::DynamicArray<uint32_t> indices(1);
        UploadBuffersToGPU(vertices, indices);
    }

    void VulkanRenderer::FrameToolsInit()
    {
        VkCommandPoolCreateInfo commandPoolsInfo {};
        commandPoolsInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolsInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow each individual command buffer to be reset
        commandPoolsInfo.queueFamilyIndex = m_graphicsQueue.index;

        VkCommandBufferAllocateInfo commandBuffersInfo{};
        commandBuffersInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBuffersInfo.pNext = nullptr;
        commandBuffersInfo.commandBufferCount = 1;
        commandBuffersInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceInfo.pNext = nullptr;

        VkSemaphoreCreateInfo semaphoresInfo{};
        semaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoresInfo.flags = 0;
        semaphoresInfo.pNext = nullptr;

        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1;
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.flags = 0;
        descriptorPoolInfo.maxSets = 1;
        descriptorPoolInfo.poolSizeCount = 1;
        descriptorPoolInfo.pPoolSizes = &poolSize;

        for(size_t i = 0; i < BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT; ++i)
        {
            FrameTools& frameTools = m_frameToolsList[i];
            VK_CHECK(vkCreateCommandPool(m_device, &commandPoolsInfo, m_pCustomAllocator, &(frameTools.mainCommandPool)));
            commandBuffersInfo.commandPool = frameTools.mainCommandPool;
            VK_CHECK(vkAllocateCommandBuffers(m_device, &commandBuffersInfo, &(frameTools.commandBuffer)));

            VK_CHECK(vkCreateFence(m_device, &fenceInfo, m_pCustomAllocator, &(frameTools.inFlightFence)))
            VK_CHECK(vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator, &(frameTools.imageAcquiredSemaphore)))
            VK_CHECK(vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator, &(frameTools.readyToPresentSemaphore)))

            CreateBuffer(m_allocator, frameTools.globalShaderDataBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
            sizeof(GlobalShaderData), VMA_ALLOCATION_CREATE_MAPPED_BIT);
            VK_CHECK(vkCreateDescriptorPool(m_device, &descriptorPoolInfo, m_pCustomAllocator, &(frameTools.globalShaderDataDescriptorPool)))
        }
    }

    void VulkanRenderer::UploadBuffersToGPU(BlitCL::DynamicArray<BlitML::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices)
    {
        // Create a storage buffer that will hold the vertices and get its device address to access it in the shaders
        VkDeviceSize vertexBufferSize = sizeof(BlitML::Vertex) * vertices.GetSize();
        CreateBuffer(m_allocator, m_globalVertexBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY, vertexBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        VkBufferDeviceAddressInfo vertexBufferAddressInfo{};
        vertexBufferAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        vertexBufferAddressInfo.pNext = nullptr;
        vertexBufferAddressInfo.buffer = m_globalVertexBuffer.buffer;
        m_globalShaderData.vertexBufferAddress = vkGetBufferDeviceAddress(m_device, &vertexBufferAddressInfo);

        // Create an index buffer that will all the loaded indices
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.GetSize();
        CreateBuffer(m_allocator, m_globalIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, indexBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);

        AllocatedBuffer stagingBuffer;
        CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, vertexBufferSize + indexBufferSize, 
        VMA_ALLOCATION_CREATE_MAPPED_BIT);
        void* pData = stagingBuffer.allocation->GetMappedData();
        BlitzenCore::BlitMemCopy(pData, vertices.Data(), vertexBufferSize);
        BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize, indices.Data(), indexBufferSize);

        BeginCommandBuffer(m_placeholderCommands, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, m_globalVertexBuffer.buffer, vertexBufferSize, 0, 0);
        CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, m_globalIndexBuffer.buffer, indexBufferSize, vertexBufferSize, 0);

        SubmitCommandBuffer(m_graphicsQueue.handle, m_placeholderCommands);

        vkQueueWaitIdle(m_graphicsQueue.handle);

        vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
    }



    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        Every operation needed for drawing a single frame is put here
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    void VulkanRenderer::DrawFrame(RenderContext& context)
    {
        if(context.windowResize)
        {
            RecreateSwapchain(context.windowWidth, context.windowHeight);
        }

        // Gets a ref to the frame tools of the current frame, so that it doesn't index into the array every time
        FrameTools& fTools = m_frameToolsList[m_currentFrame];

        // Waits for the fence in the current frame tools struct to be signaled and resets it for next time when it gets signalled
        vkWaitForFences(m_device, 1, &(fTools.inFlightFence), VK_TRUE, 1000000000);
        VK_CHECK(vkResetFences(m_device, 1, &(fTools.inFlightFence)))

        // Asks for the next image in the swapchain to use for presentation, and saves it in swapchainIdx
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_initHandles.swapchain, 1000000000, fTools.imageAcquiredSemaphore, VK_NULL_HANDLE, &swapchainIdx);

        BeginCommandBuffer(fTools.commandBuffer, 0);



        // Pipeline barrier to transtion the layout of the color attachment from undefined to general
        {
            VkImageMemoryBarrier2 colorAttachmentBarrier{};
            VkImageSubresourceRange colorAttachmentSubresource{};
            colorAttachmentSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentSubresource.baseMipLevel = 0;
            colorAttachmentSubresource.levelCount = VK_REMAINING_MIP_LEVELS;
            colorAttachmentSubresource.baseArrayLayer = 0;
            colorAttachmentSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentBarrier, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, 
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 
            colorAttachmentSubresource);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &colorAttachmentBarrier);
        }
        // Command for color attachment transition recorded



        // Clearing the color attachment
        {
            // Hard coding the color of the screen for now. At some there will be a skybox and it will be calculated with compute shaders
            VkClearColorValue clearColorAttachment{};
            clearColorAttachment.float32[0] = 0.f;
            clearColorAttachment.float32[1] = 0.f;
            clearColorAttachment.float32[2] = 0.f;
            clearColorAttachment.float32[3] = 0.f;
            VkImageSubresourceRange clearColorSubresourceRange{};
            clearColorSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            clearColorSubresourceRange.baseMipLevel = 0;
            clearColorSubresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            clearColorSubresourceRange.baseArrayLayer = 0;
            clearColorSubresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            vkCmdClearColorImage(fTools.commandBuffer, m_colorAttachment.image, VK_IMAGE_LAYOUT_GENERAL, &clearColorAttachment, 1, &clearColorSubresourceRange);
        }



        // Transition the layout of the depth attachment and color attachment to be used as rendering attachments
        {
            VkImageMemoryBarrier2 colorAttachmentBarrier{};
            VkImageSubresourceRange colorAttachmentSubresource{};
            colorAttachmentSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentSubresource.baseMipLevel = 0;
            colorAttachmentSubresource.levelCount = VK_REMAINING_MIP_LEVELS;
            colorAttachmentSubresource.baseArrayLayer = 0;
            colorAttachmentSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentBarrier, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | 
            VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | 
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, colorAttachmentSubresource);

            VkImageMemoryBarrier2 depthAttachmentBarrier{};
            VkImageSubresourceRange depthAttachmentSR{};
            depthAttachmentSR.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthAttachmentSR.baseMipLevel = 0;
            depthAttachmentSR.levelCount = VK_REMAINING_MIP_LEVELS;
            depthAttachmentSR.baseArrayLayer = 0;
            depthAttachmentSR.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_depthAttachment.image, depthAttachmentBarrier, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, 
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depthAttachmentSR);

            VkImageMemoryBarrier2 memoryBarriers[2] = {colorAttachmentBarrier, depthAttachmentBarrier};

            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, memoryBarriers);
        }
        // Attachments ready for rendering




        // Define the attachments and begin rendering
        {
            VkRenderingAttachmentInfo colorAttachment{};
            CreateRenderingAttachmentInfo(colorAttachment, m_colorAttachment.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
            VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
            VkRenderingAttachmentInfo depthAttachment{};
            CreateRenderingAttachmentInfo(depthAttachment, m_depthAttachment.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, {0, 0, 0, 0}, {0.f, 0});

            VkRenderingInfo renderingInfo{};
            renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
            renderingInfo.flags = 0;
            renderingInfo.pNext = nullptr;
            renderingInfo.viewMask = 0;
            renderingInfo.layerCount = 1;
            renderingInfo.renderArea.offset = {0, 0};
            renderingInfo.renderArea.extent = {m_drawExtent};
            renderingInfo.colorAttachmentCount = 1;
            renderingInfo.pColorAttachments = &colorAttachment;
            renderingInfo.pDepthAttachment = &depthAttachment;
            renderingInfo.pStencilAttachment = nullptr;
            vkCmdBeginRendering(fTools.commandBuffer, &renderingInfo);
        }
        // Begin rendering command recorded



        // Declaring it outside the below scope, as it needs to be bound later
        VkDescriptorSet globalShaderDataSet;
        /* Allocating and updating the descriptor set used for global shader data*/
        {
            vkResetDescriptorPool(m_device, fTools.globalShaderDataDescriptorPool, 0);
            GlobalShaderData* pGlobalShaderDataBufferData = reinterpret_cast<GlobalShaderData*>(fTools.globalShaderDataBuffer.allocation->GetMappedData());
            *pGlobalShaderDataBufferData = m_globalShaderData;
            AllocateDescriptorSets(m_device, fTools.globalShaderDataDescriptorPool, &m_globalShaderDataLayout, 1, &globalShaderDataSet);
            VkDescriptorBufferInfo globalShaderDataDescriptorBufferInfo{};
            VkWriteDescriptorSet globalShaderDataWrite{};
            WriteBufferDescriptorSets(globalShaderDataWrite, globalShaderDataDescriptorBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            globalShaderDataSet, 1, fTools.globalShaderDataBuffer.buffer, 0, VK_WHOLE_SIZE);
            vkUpdateDescriptorSets(m_device, 1, &globalShaderDataWrite, 0, nullptr);

            vkCmdBindDescriptorSets(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipelineLayout, 0, 
            1, &globalShaderDataSet, 0, nullptr);
            vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipeline);
            vkCmdBindIndexBuffer(fTools.commandBuffer, m_globalIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        }/* Commands for global shader data descriptor set recorded */



        // Dynamic viewport so I have to do this right here
        {
            VkViewport viewport{};
            viewport.x = 0;
            viewport.y = 0;
            viewport.width = static_cast<float>(m_drawExtent.width);
            viewport.height = static_cast<float>(m_drawExtent.height);
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;
            vkCmdSetViewport(fTools.commandBuffer, 0, 1, &viewport);
            VkRect2D scissor{};
            scissor.extent.width = m_drawExtent.width;
            scissor.extent.height = m_drawExtent.height;
            scissor.offset.x = 0;
            scissor.offset.y = 0;
            vkCmdSetScissor(fTools.commandBuffer, 0, 1, &scissor);
        }

        //vkCmdDrawIndexed(fTools.commandBuffer, 6, 1, 0, 0, 0);

        vkCmdEndRendering(fTools.commandBuffer);


        // Copying the color attachment to the swapchain image and transitioning the image to present
        {
            // color attachment barrier
            VkImageMemoryBarrier2 colorAttachmentBarrier{};
            VkImageSubresourceRange colorAttachmentSubresource{};
            colorAttachmentSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentSubresource.baseMipLevel = 0;
            colorAttachmentSubresource.levelCount = VK_REMAINING_MIP_LEVELS;
            colorAttachmentSubresource.baseArrayLayer = 0;
            colorAttachmentSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentBarrier, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | 
            VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, colorAttachmentSubresource);

            //swapchain image barrier
            VkImageMemoryBarrier2 swapchainImageBarrier{};
            VkImageSubresourceRange swapchainImageSR{};
            swapchainImageSR.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            swapchainImageSR.baseMipLevel = 0;
            swapchainImageSR.levelCount = VK_REMAINING_MIP_LEVELS;
            swapchainImageSR.baseArrayLayer = 0;
            swapchainImageSR.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], swapchainImageBarrier, VK_PIPELINE_STAGE_2_NONE, 
            VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, swapchainImageSR);

            VkImageMemoryBarrier2 firstTransferMemoryBarriers[2] = {colorAttachmentBarrier, swapchainImageBarrier};

            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, firstTransferMemoryBarriers);

            // Copy the color attachment to the swapchain image
            VkImageSubresourceLayers colorAttachmentSL{};
            colorAttachmentSL.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentSL.baseArrayLayer = 0;
            colorAttachmentSL.layerCount = 1;
            colorAttachmentSL.mipLevel = 0;
            VkImageSubresourceLayers swapchainImageSL{};
            swapchainImageSL.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            swapchainImageSL.baseArrayLayer = 0;
            swapchainImageSL.layerCount = 1;
            swapchainImageSL.mipLevel = 0;
            CopyImageToImage(fTools.commandBuffer, m_colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
            m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_drawExtent, 
            m_initHandles.swapchainExtent, colorAttachmentSL, swapchainImageSL);

            // Change the swapchain image layout to present
            VkImageMemoryBarrier2 presentImageBarrier{};
            VkImageSubresourceRange presentImageSR{};
            presentImageSR.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            presentImageSR.baseMipLevel = 0;
            presentImageSR.levelCount = VK_REMAINING_MIP_LEVELS;
            presentImageSR.baseArrayLayer = 0;
            presentImageSR.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], presentImageBarrier, VK_PIPELINE_STAGE_2_BLIT_BIT, 
            VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, presentImageSR);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);

        }
        // Swapchain image ready to be presented

        SubmitCommandBuffer(m_graphicsQueue.handle, fTools.commandBuffer, 1, fTools.imageAcquiredSemaphore, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, 
        1, fTools.readyToPresentSemaphore, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, fTools.inFlightFence);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &fTools.readyToPresentSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &(m_initHandles.swapchain);
        presentInfo.pImageIndices = &swapchainIdx;
        vkQueuePresentKHR(m_presentQueue.handle, &presentInfo);

        m_currentFrame = (m_currentFrame + 1) % BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT;
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

    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, uint8_t waitSemaphoreCount /* =0 */, 
    VkSemaphore waitSemaphore /* =VK_NULL_HANDLE */, VkPipelineStageFlags2 waitPipelineStage /*=VK_PIPELINE_STAGE_2_NONE*/, uint8_t signalSemaphoreCount /* =0 */,
    VkSemaphore signalSemaphore /* =VK_NULL_HANDLE */, VkPipelineStageFlags2 signalPipelineStage /*=VK_PIPELINE_STAGE_2_NONE*/, VkFence fence /* =VK_NULL_HANDLE */)
    {
        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        VkSemaphoreSubmitInfo waitSemaphoreInfo{};
        waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        waitSemaphoreInfo.stageMask = waitPipelineStage;
        waitSemaphoreInfo.semaphore = waitSemaphore;

        VkSemaphoreSubmitInfo signalSemaphoreInfo{};
        signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalSemaphoreInfo.stageMask = signalPipelineStage;
        signalSemaphoreInfo.semaphore = signalSemaphore;

        VkCommandBufferSubmitInfo commandBufferInfo{};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferInfo.commandBuffer = commandBuffer;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &commandBufferInfo;
        submitInfo.waitSemaphoreInfoCount = waitSemaphoreCount;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
        submitInfo.signalSemaphoreInfoCount = signalSemaphoreCount;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;
        vkQueueSubmit2(queue, 1, &submitInfo, fence);
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

    void VulkanRenderer::RecreateSwapchain(uint32_t windowWidth, uint32_t windowHeight)
    {
        //Wait for the current frame to be done with the swapchain
        vkDeviceWaitIdle(m_device);

        //Destroy the current swapchain
        for(size_t i = 0; i < m_initHandles.swapchainImageViews.GetSize(); ++i)
        {
            vkDestroyImageView(m_device, m_initHandles.swapchainImageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_initHandles.swapchain, nullptr);

        CreateSwapchain(m_device, m_initHandles, windowWidth, windowHeight, m_graphicsQueue, 
        m_presentQueue, m_computeQueue, m_pCustomAllocator);

        //The draw extent should also be updated depending on if the swapchain got bigger or smaller
        m_drawExtent.width = std::min(static_cast<uint32_t>(windowWidth), 
        static_cast<uint32_t>(m_colorAttachment.extent.width));
        m_drawExtent.height = std::min(static_cast<uint32_t>(windowHeight), 
        static_cast<uint32_t>(m_colorAttachment.extent.height));
    }
}