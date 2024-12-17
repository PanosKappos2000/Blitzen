#include "vulkanRenderer.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

void DrawMeshTastsIndirectNv(VkInstance instance, VkCommandBuffer commandBuffer, VkBuffer indirectBuffer, 
VkDeviceSize offset, uint32_t drawCount, uint32_t stride) 
{
    auto func = (PFN_vkCmdDrawMeshTasksIndirectNV) vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectNV");
    if (func != nullptr) 
    {
        func(commandBuffer, indirectBuffer, offset, drawCount, stride);
    } 
}

namespace BlitzenVulkan
{
    void VulkanRenderer::UploadDataToGPUAndSetupForRendering(GPUData& gpuData)
    {
        // Setting the size of the texture array here , so that the pipeline knows how many descriptors the layout should have
        m_currentStaticBuffers.loadedTextures.Resize(gpuData.textureCount);

        // Creating the layout here so that any descriptor sets that are created know about it
        {
            // Descriptor set layout for global shader data
            VkDescriptorSetLayoutBinding shaderDataLayoutBinding{};
            VkDescriptorSetLayoutBinding bufferAddressBinding{};
            if(m_stats.meshShaderSupport)
            {
                CreateDescriptorSetLayoutBinding(shaderDataLayoutBinding, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT);
                CreateDescriptorSetLayoutBinding(bufferAddressBinding, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            }
            else
            {
                CreateDescriptorSetLayoutBinding(shaderDataLayoutBinding, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
                CreateDescriptorSetLayoutBinding(bufferAddressBinding, 1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
            }
            
            VkDescriptorSetLayoutBinding shaderDataBindings[2] = {shaderDataLayoutBinding, bufferAddressBinding};
            m_globalShaderDataLayout = CreateDescriptorSetLayout(m_device, 2, shaderDataBindings);

            // Descriptor set layout for textures
            VkDescriptorSetLayoutBinding texturesLayoutBinding{};
            CreateDescriptorSetLayoutBinding(texturesLayoutBinding, 0, static_cast<uint32_t>(m_currentStaticBuffers.loadedTextures.GetSize()), 
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
            m_currentStaticBuffers.textureDescriptorSetlayout = CreateDescriptorSetLayout(m_device, 1, &texturesLayoutBinding);

            VkDescriptorSetLayout layouts [2] = {m_globalShaderDataLayout, m_currentStaticBuffers.textureDescriptorSetlayout};

            // I'm not using the push constants anymore(but I could use it for buffer addrs)
            VkPushConstantRange pushConstant{};
            CreatePushConstantRange(pushConstant, VK_SHADER_STAGE_VERTEX_BIT, sizeof(ShaderPushConstant), 0);

            CreatePipelineLayout(m_device, &m_opaqueGraphicsPipelineLayout, 2, layouts, 1, &pushConstant);
        }

        // Texture sampler, for now all textures will use the same one
        CreateTextureSampler(m_device, m_placeholderSampler);

        // Allocating an image for each texture and telling it to use the one sampler the renderer creates (for now)
        for(size_t i = 0; i < gpuData.textureCount; ++i)
        {
            
            CreateTextureImage(reinterpret_cast<void*>(gpuData.pTextures[i].pTextureData), m_device, m_allocator, 
            m_currentStaticBuffers.loadedTextures[i].image, 
            {(uint32_t)gpuData.pTextures[i].textureWidth, (uint32_t)gpuData.pTextures[i].textureHeight, 1}, VK_FORMAT_R8G8B8A8_UNORM, 
            VK_IMAGE_USAGE_SAMPLED_BIT, m_placeholderCommands, m_graphicsQueue.handle, 0);

            m_currentStaticBuffers.loadedTextures[i].sampler = m_placeholderSampler;
        }

        // This holds data that maps to one draw call for one surface 
        // For now it will be used to render many instances of the same mesh
        BlitCL::DynamicArray<StaticRenderObject> renderObjects(10000);
        BlitCL::DynamicArray<IndirectDrawData> indirectDraws(10000);
        for(size_t i = 0; i < 9995; ++i)
        {
            BlitzenEngine::PrimitiveSurface& currentSurface = gpuData.pMeshes[0].surfaces[0];
            IndirectDrawData& currentIDraw = indirectDraws[i];
            StaticRenderObject& currentObject = renderObjects[i];

            currentObject.materialTag = currentSurface.pMaterial->materialTag;
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 40 - 20,//x 
            (float(rand()) / RAND_MAX) * 40 - 20,//y
            (float(rand()) / RAND_MAX) * 40 - 20);//z
            currentObject.pos = translation;
            currentObject.scale = 5.0f;

            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
            (float(rand()) / RAND_MAX) * 2 - 1, // y
            (float(rand()) / RAND_MAX) * 2 - 1); // z
		    float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            currentObject.orientation = orientation;

            currentIDraw.drawIndirect.firstIndex = currentSurface.firstIndex;
            currentIDraw.drawIndirect.indexCount = currentSurface.indexCount;
            currentIDraw.drawIndirect.vertexOffset = currentSurface.vertexOffset;
            currentIDraw.drawIndirect.instanceCount = 1;
            currentIDraw.drawIndirect.firstInstance = 0;

            currentIDraw.drawIndirectTasks.firstTask = currentSurface.firstMeshlet;
            currentIDraw.drawIndirectTasks.taskCount = currentSurface.meshletCount;
        }

        for (size_t i = 9995; i < 10000; ++i)
        {
            BlitzenEngine::PrimitiveSurface& currentSurface = gpuData.pMeshes[1].surfaces[0];
            IndirectDrawData& currentIDraw = indirectDraws[i];
            StaticRenderObject& currentObject = renderObjects[i];

            currentObject.materialTag = currentSurface.pMaterial->materialTag;
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 40 - 20,//x 
                (float(rand()) / RAND_MAX) * 40 - 20,//y
                (float(rand()) / RAND_MAX) * 40 - 20);//z
            currentObject.pos = translation;
            currentObject.scale = 0.1f;

            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
            float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            currentObject.orientation = orientation;

            currentIDraw.drawIndirect.firstIndex = currentSurface.firstIndex;
            currentIDraw.drawIndirect.indexCount = currentSurface.indexCount;
            currentIDraw.drawIndirect.vertexOffset = currentSurface.vertexOffset;
            currentIDraw.drawIndirect.instanceCount = 1;
            currentIDraw.drawIndirect.firstInstance = 0;

            currentIDraw.drawIndirectTasks.firstTask = currentSurface.firstMeshlet;
            currentIDraw.drawIndirectTasks.taskCount = currentSurface.meshletCount;
        }

        // This will hold all the data needed to record all the draw commands indirectly, using a GPU buffer
        /*BlitCL::DynamicArray<VkDrawIndexedIndirectCommand> indirectDraws;
        for(size_t i = 0; i < gpuData.meshCount; ++i)
        {
            // Since there will only be one array that holds all the data for each surfaces array
            // The indirect draw array needs to be resized and write to the parts after the previous last element
            size_t previousIndirectDrawSize = indirectDraws.GetSize();
            indirectDraws.Resize(gpuData.pMeshes[i].surfaces.GetSize() + previousIndirectDrawSize);

            for(size_t s = 0; s < gpuData.pMeshes[i].surfaces.GetSize(); ++s)
            {
                BlitzenEngine::PrimitiveSurface& currentSurface = gpuData.pMeshes[i].surfaces[s];
                VkDrawIndexedIndirectCommand& currentIDraw = indirectDraws[previousIndirectDrawSize + s];

                // A VkDrawIndexedIndirectCommand holds all the data needed for one vkCmdDrawIndexed command
                // This data can be retrieved from each surface that a mesh has and needs to be drawn
                currentIDraw.firstIndex = currentSurface.firstIndex;
                currentIDraw.indexCount = currentSurface.indexCount;
                currentIDraw.instanceCount = 1000;
                currentIDraw.firstInstance = 0;
                currentIDraw.vertexOffset = 0;
            }
        } This is the correct code compared to the hardcoded one above, I will restore it later*/

        // Configure the material data to what is actually needed by the GPU
        BlitCL::DynamicArray<MaterialConstants> materials(gpuData.materialCount);
        for(size_t i = 0; i < gpuData.materialCount; ++i)
        {
            materials[i].diffuseColor = gpuData.pMaterials[i].diffuseColor;
            materials[i].diffuseTextureTag = gpuData.pMaterials[i].diffuseMapTag;
        }

        if (m_stats.meshShaderSupport)
            UploadDataToGPU(gpuData.vertices, gpuData.indices, renderObjects, materials, gpuData.meshlets, indirectDraws);
        else
            UploadDataToGPU(gpuData.vertices, gpuData.indices, renderObjects, materials, gpuData.meshlets, indirectDraws);


        /* Compute pipeline that fills the draw indirect commands based on culling data*/
        {
            VkComputePipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.flags = 0;
            pipelineInfo.pNext = nullptr;

            VkShaderModule computeShaderModule{};
            VkPipelineShaderStageCreateInfo shaderStageInfo{};
            BlitCL::DynamicArray<char> code;
            CreateShaderProgram(m_device, "VulkanShaders/IndirectCulling.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", computeShaderModule, 
            shaderStageInfo, &code);

            pipelineInfo.stage = shaderStageInfo;
            pipelineInfo.layout = m_opaqueGraphicsPipelineLayout;// This layout seems adequate for the compute shader inspite of its name

            VK_CHECK(vkCreateComputePipelines(m_device, nullptr, 1, &pipelineInfo, m_pCustomAllocator, &m_indirectCullingComputePipeline));

            // Beyond this scope, this shader module is not needed
            vkDestroyShaderModule(m_device, computeShaderModule, m_pCustomAllocator);
        }




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
            if(m_stats.meshShaderSupport)
            {
                CreateShaderProgram(m_device, "VulkanShaders/MeshShader.mesh.glsl.spv", VK_SHADER_STAGE_MESH_BIT_NV, "main", vertexShaderModule, 
                shaderStages[0], nullptr);
            }
            else
            {
                CreateShaderProgram(m_device, "VulkanShaders/MainObjectShader.vert.glsl.spv", VK_SHADER_STAGE_VERTEX_BIT, "main", vertexShaderModule,
                shaderStages[0], nullptr);
            }
             
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
            SetupDepthTest(depthState, VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_TRUE, VK_FALSE, 0.f, 0.f, VK_FALSE, 
            nullptr, nullptr);
            pipelineInfo.pDepthStencilState = &depthState;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            CreateColorBlendAttachment(colorBlendAttachment, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_CONSTANT_ALPHA, 
            VK_BLEND_FACTOR_CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA);
            VkPipelineColorBlendStateCreateInfo colorBlendState{};
            CreateColorBlendState(colorBlendState, 1, &colorBlendAttachment, VK_FALSE, VK_LOGIC_OP_AND);
            pipelineInfo.pColorBlendState = &colorBlendState;

            VkPipelineVertexInputStateCreateInfo vertexInput{};
            vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            pipelineInfo.pVertexInputState = &vertexInput;
            pipelineInfo.layout = m_opaqueGraphicsPipelineLayout;

            VK_CHECK(vkCreateGraphicsPipelines(m_device, nullptr, 1, &pipelineInfo, m_pCustomAllocator, &m_opaqueGraphicsPipeline))

            vkDestroyShaderModule(m_device, vertexShaderModule, m_pCustomAllocator);
            vkDestroyShaderModule(m_device, fragShaderModule, m_pCustomAllocator);
        }
    }

    void VulkanRenderer::UploadDataToGPU(BlitCL::DynamicArray<BlitML::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices, 
    BlitCL::DynamicArray<StaticRenderObject>& staticObjects, BlitCL::DynamicArray<MaterialConstants>& materials, 
    BlitCL::DynamicArray<BlitML::Meshlet>& meshlets, BlitCL::DynamicArray<IndirectDrawData>& indirectDraws)
    {
        // Create a storage buffer that will hold the vertices and get its device address to access it in the shaders
        VkDeviceSize vertexBufferSize = sizeof(BlitML::Vertex) * vertices.GetSize();
        CreateBuffer(m_allocator, m_currentStaticBuffers.globalVertexBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, vertexBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        // This buffer will live in the GPU but its address will be retrieved, so that it can be accessed in the shader
        m_currentStaticBuffers.bufferAddresses.vertexBufferAddress = 
        GetBufferAddress(m_device, m_currentStaticBuffers.globalVertexBuffer.buffer);

        // Create an index buffer that will hold all the loaded indices
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.GetSize();
        // Like the vertex buffer, this will live in the GPU but it does not need to be accessed in the shader, only bound before drawing
        CreateBuffer(m_allocator, m_currentStaticBuffers.globalIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, indexBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);

        // The render object buffer will hold per object data for all objects in the scene
        VkDeviceSize renderObjectBufferSize = sizeof(StaticRenderObject) * staticObjects.GetSize();
        CreateBuffer(m_allocator, m_currentStaticBuffers.renderObjectBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, renderObjectBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        // It also lives in the GPU and needs to be accessed in the shader for per object data like position and orientation
        m_currentStaticBuffers.bufferAddresses.renderObjectBufferAddress = 
        GetBufferAddress(m_device, m_currentStaticBuffers.renderObjectBuffer.buffer);

        // Holds material constants 
        // (like diffuse color and and indices into the array of textures for the different texture maps it uses) 
        // for every material
        VkDeviceSize materialBufferSize = sizeof(MaterialConstants) * materials.GetSize();
        CreateBuffer(m_allocator, m_currentStaticBuffers.globalMaterialBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, materialBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        // Lives in the GPU and will be accessed by the vertex shader to pass the material tag to the fragment shader
        m_currentStaticBuffers.bufferAddresses.materialBufferAddress = 
        GetBufferAddress(m_device, m_currentStaticBuffers.globalMaterialBuffer.buffer);

        // Holds all indirect commands needed for vkCmdIndirectDraw variants to draw everything in a scene
        VkDeviceSize indirectBufferSize = sizeof(IndirectDrawData) * indirectDraws.GetSize();
        // Aside from an indirect buffer, it is also used as a storge buffer as its data will be read and manipulated by compute shaders
        CreateBuffer(m_allocator, m_currentStaticBuffers.drawIndirectBuffer, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, indirectBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        // As explained above this buffer will be accessed by compute shaders through device address
        m_currentStaticBuffers.bufferAddresses.indirectBufferAddress = 
        GetBufferAddress(m_device, m_currentStaticBuffers.drawIndirectBuffer.buffer);

        // The same as the above but this one will be the final one which will be processed by the compute shaders
        // Normally, I wouldn't copy the data on to this one but the compute shader fails to setup the commands properly
        CreateBuffer(m_allocator, m_currentStaticBuffers.drawIndirectBufferFinal, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, indirectBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        m_currentStaticBuffers.bufferAddresses.finalIndirectBufferAddress =
        GetBufferAddress(m_device, m_currentStaticBuffers.drawIndirectBufferFinal.buffer);

        // This holds the combined size of all the required buffers to pass to the combined staging buffer
        VkDeviceSize stagingBufferSize = vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize + indirectBufferSize;

        VkDeviceSize meshBufferSize = sizeof(BlitML::Meshlet) * meshlets.GetSize();// Defining this here so that it does not go out of scope
        // The mesh/meshlet buffer will only be created if mesh shading is supported(checked during initalization, only if it the mesh shader macro is set to 1)
        if(m_stats.meshShaderSupport)
        {
            // Holds per object meshlet data for all the objects in the shader
            CreateBuffer(m_allocator, m_currentStaticBuffers.globalMeshBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
            VMA_MEMORY_USAGE_GPU_ONLY, meshBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
            // Like most of the above, it will be accessed in the shaders
            m_currentStaticBuffers.bufferAddresses.meshBufferAddress = 
            GetBufferAddress(m_device, m_currentStaticBuffers.globalMeshBuffer.buffer);
            // the staging buffer size will also be incremented if the meshlet buffer is created
            stagingBufferSize += meshBufferSize;
        }

        AllocatedBuffer stagingBuffer;
        CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
        stagingBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        void* pData = stagingBuffer.allocation->GetMappedData();
        BlitzenCore::BlitMemCopy(pData, vertices.Data(), vertexBufferSize);
        BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize, indices.Data(), indexBufferSize);
        BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize + indexBufferSize, staticObjects.Data(), 
        renderObjectBufferSize);
        BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize + indexBufferSize + renderObjectBufferSize, 
        materials.Data(), materialBufferSize);
        BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize, 
        indirectDraws.Data(), indirectBufferSize);

        if(m_stats.meshShaderSupport)
        {
            BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize
            + indirectBufferSize, meshlets.Data(), meshBufferSize);
        }

        BeginCommandBuffer(m_placeholderCommands, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, 
        m_currentStaticBuffers.globalVertexBuffer.buffer, vertexBufferSize, 
        0, 0);

        CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, 
        m_currentStaticBuffers.globalIndexBuffer.buffer, indexBufferSize, 
        vertexBufferSize, 0);

        CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, 
        m_currentStaticBuffers.renderObjectBuffer.buffer, renderObjectBufferSize, 
        vertexBufferSize + indexBufferSize, 0);

        CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, 
        m_currentStaticBuffers.globalMaterialBuffer.buffer, materialBufferSize, 
        vertexBufferSize + indexBufferSize + renderObjectBufferSize, 0);

        CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, 
        m_currentStaticBuffers.drawIndirectBuffer.buffer, indirectBufferSize, 
        vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize, 0);

        /*CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, 
        m_currentStaticBuffers.drawIndirectBufferFinal.buffer, indirectBufferSize, 
        vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize, 0);*/

        if(m_stats.meshShaderSupport)
        {
            CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, 
            m_currentStaticBuffers.globalMeshBuffer.buffer, meshBufferSize, 
            vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize + indirectBufferSize, 0);
        }
        

        SubmitCommandBuffer(m_graphicsQueue.handle, m_placeholderCommands);

        vkQueueWaitIdle(m_graphicsQueue.handle);

        vmaDestroyBuffer(m_allocator, stagingBuffer.buffer, stagingBuffer.allocation);


        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        // This would normally be 1, but an old machine that I tested this on, failed because of this. I will look at the documentation for an explanation
        poolSize.descriptorCount = 5;
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.pNext = nullptr;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, m_pCustomAllocator, &m_currentStaticBuffers.textureDescriptorPool))
 
        AllocateDescriptorSets(m_device, m_currentStaticBuffers.textureDescriptorPool, &m_currentStaticBuffers.textureDescriptorSetlayout, 
        1, &m_currentStaticBuffers.textureDescriptorSet);

        BlitCL::DynamicArray<VkDescriptorImageInfo> imageInfos(m_currentStaticBuffers.loadedTextures.GetSize());
        for(size_t i = 0; i < imageInfos.GetSize(); ++i)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = m_currentStaticBuffers.loadedTextures[i].image.imageView;
            imageInfos[i].sampler = m_currentStaticBuffers.loadedTextures[i].sampler;
        }

        VkWriteDescriptorSet write{};
        WriteImageDescriptorSets(write, imageInfos.Data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_currentStaticBuffers.textureDescriptorSet, 
        static_cast<uint32_t>(imageInfos.GetSize()), 0);
        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
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

        // The calculation will be moved to the camera / engine soon 
        m_globalShaderData.projection = context.projectionMatrix;
        m_globalShaderData.view = context.viewMatrix;
        m_globalShaderData.projectionView = context.projectionView;
        m_globalShaderData.viewPosition = context.viewPosition;
        m_globalShaderData.sunlightDir = context.sunlightDirection;
        m_globalShaderData.sunlightColor = context.sunlightColor;

        // Declaring it outside the below scope, as it needs to be bound later
        VkDescriptorSet globalShaderDataSet;
        /* Allocating and updating the descriptor set used for global shader data*/
        {
            vkResetDescriptorPool(m_device, fTools.globalShaderDataDescriptorPool, 0);
            GlobalShaderData* pGlobalShaderDataBufferData = reinterpret_cast<GlobalShaderData*>(fTools.globalShaderDataBuffer.allocation->GetMappedData());
            *pGlobalShaderDataBufferData = m_globalShaderData;
            BufferDeviceAddresses* pAddressBufferPointer = reinterpret_cast<BufferDeviceAddresses*>(fTools.bufferDeviceAddrsBuffer.allocation->GetMappedData());
            *pAddressBufferPointer = m_currentStaticBuffers.bufferAddresses;
            AllocateDescriptorSets(m_device, fTools.globalShaderDataDescriptorPool, &m_globalShaderDataLayout, 1, &globalShaderDataSet);
            VkDescriptorBufferInfo globalShaderDataDescriptorBufferInfo{};
            VkWriteDescriptorSet globalShaderDataWrite{};
            WriteBufferDescriptorSets(globalShaderDataWrite, globalShaderDataDescriptorBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            globalShaderDataSet, 0, 1, fTools.globalShaderDataBuffer.buffer, 0, VK_WHOLE_SIZE);
            VkDescriptorBufferInfo bufferAddressDescriptorBufferInfo{};
            VkWriteDescriptorSet bufferAddressDescriptorWrite{};
            WriteBufferDescriptorSets(bufferAddressDescriptorWrite, bufferAddressDescriptorBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, globalShaderDataSet, 
            1, 1, fTools.bufferDeviceAddrsBuffer.buffer, 0, sizeof(BufferDeviceAddresses));
            VkWriteDescriptorSet writes[2] = {globalShaderDataWrite, bufferAddressDescriptorWrite};
            vkUpdateDescriptorSets(m_device, 2, writes, 0, nullptr);
        }/* Commands for global shader data descriptor set recorded */

        // Asks for the next image in the swapchain to use for presentation, and saves it in swapchainIdx
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_initHandles.swapchain, 1000000000, fTools.imageAcquiredSemaphore, VK_NULL_HANDLE, &swapchainIdx);

        BeginCommandBuffer(fTools.commandBuffer, 0);



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
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, colorAttachmentSubresource);

            VkImageMemoryBarrier2 depthAttachmentBarrier{};
            VkImageSubresourceRange depthAttachmentSR{};
            depthAttachmentSR.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthAttachmentSR.baseMipLevel = 0;
            depthAttachmentSR.levelCount = VK_REMAINING_MIP_LEVELS;
            depthAttachmentSR.baseArrayLayer = 0;
            depthAttachmentSR.layerCount = VK_REMAINING_ARRAY_LAYERS;
            ImageMemoryBarrier(m_depthAttachment.image, depthAttachmentBarrier, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_MEMORY_READ_BIT |
                VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, depthAttachmentSR);

            VkImageMemoryBarrier2 memoryBarriers[2] = {colorAttachmentBarrier, depthAttachmentBarrier};
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, memoryBarriers);
        }
        // Attachments ready for rendering




        /* Dispatching the compute shader that fill the indirect buffer that will be used by draw indirect*/
        {
            vkCmdBindDescriptorSets(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_opaqueGraphicsPipelineLayout, 
            0, 1, &globalShaderDataSet, 0, nullptr);

            vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_indirectCullingComputePipeline);
            vkCmdDispatch(fTools.commandBuffer, static_cast<uint32_t>((context.drawCount / 256) + 1), 1, 1);

            VkMemoryBarrier2 memoryBarrier{};
            MemoryBarrier(memoryBarrier, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_MEMORY_READ_BIT_KHR);

            // Stops later stages from reading from this buffer while compute is not done. Could add this below with the other barriers
            PipelineBarrier(fTools.commandBuffer, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
        }




        // Define the attachments and begin rendering
        {
            VkRenderingAttachmentInfo colorAttachment{};
            CreateRenderingAttachmentInfo(colorAttachment, m_colorAttachment.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, {0.1f, 0.2f, 0.3f, 0});
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




        // Dynamic viewport so I have to do this right here
        {
            VkViewport viewport{};
            viewport.x = 0;
            viewport.y = static_cast<float>(m_drawExtent.height); // Start from full height (flips y axis)
            viewport.width = static_cast<float>(m_drawExtent.width);
            viewport.height = -static_cast<float>(m_drawExtent.height);// Move a negative amount of full height (flips y axis)
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

        VkDescriptorSet descriptorSets [2] = {globalShaderDataSet, m_currentStaticBuffers.textureDescriptorSet};
        vkCmdBindDescriptorSets(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipelineLayout, 0,
        2, descriptorSets, 0, nullptr);
        vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipeline);
        vkCmdBindIndexBuffer(fTools.commandBuffer, m_currentStaticBuffers.globalIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        if(m_stats.meshShaderSupport)
        {
            /*for(uint32_t i = 0; i < context.drawCount; ++i)
            {
            
                DrawMeshTastsNv(m_initHandles.instance, fTools.commandBuffer, context.pDraws[i].meshletCount, context.pDraws[i].firstMeshlet);
                ShaderPushConstant index;
                index.drawTag = context.pDraws[i].objectTag; // Only one object currently so I am hardcoding the index to test it
                vkCmdPushConstants(fTools.commandBuffer, m_opaqueGraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 
                sizeof(ShaderPushConstant), &index);
                vkCmdDrawIndexed(fTools.commandBuffer, context.pDraws[i].indexCount, 1, context.pDraws[i].firstIndex, 0, 0);
            }*/
           DrawMeshTastsIndirectNv(m_initHandles.instance, fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBuffer.buffer, 
           offsetof(IndirectDrawData, drawIndirectTasks), context.drawCount, sizeof(IndirectDrawData));
        }
        else
        {
            vkCmdDrawIndexedIndirect(fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBufferFinal.buffer, 
            offsetof(IndirectDrawData, drawIndirect), context.drawCount, sizeof(IndirectDrawData));// Ah, the beauty of draw indirect
        }

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
            VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, presentImageSR);
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
        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        CreateSwapchain(m_device, m_initHandles, windowWidth, windowHeight, m_graphicsQueue, 
        m_presentQueue, m_computeQueue, m_pCustomAllocator, newSwapchain, m_initHandles.swapchain);

        // The draw extent should also be updated depending on if the swapchain got bigger or smaller
        m_drawExtent.width = std::min(windowWidth, m_drawExtent.width);
        m_drawExtent.height = std::min(windowHeight, m_drawExtent.height);

        // Wait for the GPU to be done with the swapchain and destroy the swapchain
        vkDeviceWaitIdle(m_device);
        vkDestroySwapchainKHR(m_device, m_initHandles.swapchain, nullptr);
        m_initHandles.swapchain = newSwapchain;
    }
}