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
    void VulkanRenderer::VarBuffersInit()
    {
        for(size_t i = 0; i < BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VarBuffers& buffers = m_varBuffers[i];

            CreateBuffer(m_allocator, buffers.globalShaderDataBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
            sizeof(GlobalShaderData), VMA_ALLOCATION_CREATE_MAPPED_BIT);
            buffers.pGlobalShaderData = reinterpret_cast<GlobalShaderData*>(buffers.globalShaderDataBuffer.allocation->GetMappedData());

            CreateBuffer(m_allocator, buffers.bufferDeviceAddrsBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
            sizeof(BufferDeviceAddresses), VMA_ALLOCATION_CREATE_MAPPED_BIT);
            buffers.pBufferAddrs = reinterpret_cast<BufferDeviceAddresses*>(buffers.bufferDeviceAddrsBuffer.allocation->GetMappedData());

            CreateBuffer(m_allocator, buffers.cullingDataBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
            sizeof(CullingData), VMA_ALLOCATION_CREATE_MAPPED_BIT);
            buffers.pCullingData = reinterpret_cast<CullingData*>(buffers.cullingDataBuffer.allocation->GetMappedData());
        }
    }

    void VulkanRenderer::UploadDataToGPUAndSetupForRendering(GPUData& gpuData)
    {
        VarBuffersInit();

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
                VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
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

            VkDescriptorSetLayoutBinding cullingDataLayoutBinding{};
            CreateDescriptorSetLayoutBinding(cullingDataLayoutBinding, 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            VK_SHADER_STAGE_COMPUTE_BIT);

            VkDescriptorSetLayoutBinding depthImageBinding{};
            CreateDescriptorSetLayoutBinding(depthImageBinding, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            VK_SHADER_STAGE_COMPUTE_BIT);
            
            VkDescriptorSetLayoutBinding shaderDataBindings[4] = {shaderDataLayoutBinding, bufferAddressBinding, 
            cullingDataLayoutBinding, depthImageBinding};
            m_globalShaderDataLayout = CreateDescriptorSetLayout(m_device, 4, shaderDataBindings, 
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);

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

        // Descriptor set layouts for depth reduce compute shader
        {
            VkDescriptorSetLayoutBinding inImageLayoutBinding{};
            VkDescriptorSetLayoutBinding outImageLayoutBinding{};
            CreateDescriptorSetLayoutBinding(inImageLayoutBinding, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
            CreateDescriptorSetLayoutBinding(outImageLayoutBinding, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

            VkDescriptorSetLayoutBinding storageImageBindings[2] = {inImageLayoutBinding, outImageLayoutBinding};
            m_depthPyramidImageDescriptorSetLayout = CreateDescriptorSetLayout(m_device, 2, storageImageBindings, 
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
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
        BlitCL::DynamicArray<StaticRenderObject> renderObjects(1000000);
        BlitCL::DynamicArray<IndirectOffsets> indirectDraws(1000000);
        /*  
            This is temporary hardcoded loading of multiple objects for testing.
            Normally, everything below would be given by a game object defined outside the renderer 
        */
        {
            for(size_t i = 0; i < 999995; ++i)
            {
                BlitzenEngine::PrimitiveSurface& currentSurface = gpuData.pMeshes[0].surfaces[0];
                IndirectOffsets& currentIDraw = indirectDraws[i];
                StaticRenderObject& currentObject = renderObjects[i];

                currentObject.materialTag = currentSurface.pMaterial->materialTag;
                BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1000 - 50,//x 
                (float(rand()) / RAND_MAX) * 200 - 50,//y
                (float(rand()) / RAND_MAX) * 1000 - 50);//z
                currentObject.pos = translation;
                currentObject.scale = 1.f;

                BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
		        float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
                BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
                currentObject.orientation = orientation;

                currentObject.center = currentSurface.center;
                currentObject.radius = currentSurface.radius;

                for(size_t i = 0; i < currentSurface.lodCount; ++i)
                    currentIDraw.lod[i] = currentSurface.meshLod[i];
                currentIDraw.lodCount = currentSurface.lodCount;
                currentIDraw.vertexOffset = currentSurface.vertexOffset;

                currentIDraw.firstTask = currentSurface.firstMeshlet;
                currentIDraw.taskCount = currentSurface.meshletCount;
            }

            for (size_t i = 999000; i < 1000000; ++i)
            {
                BlitzenEngine::PrimitiveSurface& currentSurface = gpuData.pMeshes[1].surfaces[0];
                IndirectOffsets& currentIDraw = indirectDraws[i];
                StaticRenderObject& currentObject = renderObjects[i];

                currentObject.materialTag = currentSurface.pMaterial->materialTag;
                BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1000 - 20,//x 
                    (float(rand()) / RAND_MAX) * 200 - 20,//y
                    (float(rand()) / RAND_MAX) * 1000 - 20);//z
                currentObject.pos = translation;
                currentObject.scale = 0.1f;

                BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                    (float(rand()) / RAND_MAX) * 2 - 1, // y
                    (float(rand()) / RAND_MAX) * 2 - 1); // z
                float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
                BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
                currentObject.orientation = orientation;

                currentObject.center = currentSurface.center;
                currentObject.radius = currentSurface.radius;

                for(size_t i = 0; i < currentIDraw.lodCount; ++i)
                    currentIDraw.lod[i] = currentSurface.meshLod[i];
                currentIDraw.lodCount = currentSurface.lodCount;
                currentIDraw.vertexOffset = currentSurface.vertexOffset;

                currentIDraw.firstTask = currentSurface.firstMeshlet;
                currentIDraw.taskCount = currentSurface.meshletCount;
            }
        }


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
            CreateComputeShaderProgram(m_device, "VulkanShaders/IndirectCulling.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
            m_opaqueGraphicsPipelineLayout, &m_indirectCullingComputePipeline);
        }

        {
            VkPushConstantRange pushConstant{};
            CreatePushConstantRange(pushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(ShaderPushConstant));
            CreatePipelineLayout(m_device, &m_depthReducePipelineLayout, 1, &m_depthPyramidImageDescriptorSetLayout, 1, &pushConstant);

            CreateComputeShaderProgram(m_device, "VulkanShaders/DepthReduce.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
            m_depthReducePipelineLayout, &m_depthReduceComputePipeline);
        }

        {
            CreatePipelineLayout(m_device, &m_lateCullingPipelineLayout, 1, &m_globalShaderDataLayout, 0, nullptr);

            CreateComputeShaderProgram(m_device, "VulkanShaders/LateCulling.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
            m_lateCullingPipelineLayout, &m_lateCullingComputePipeline);
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
                shaderStages[0]);
            }
            else
            {
                CreateShaderProgram(m_device, "VulkanShaders/MainObjectShader.vert.glsl.spv", VK_SHADER_STAGE_VERTEX_BIT, "main", vertexShaderModule,
                shaderStages[0]);
            }
             
            VkShaderModule fragShaderModule;
            CreateShaderProgram(m_device, "VulkanShaders/MainObjectShader.frag.glsl.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragShaderModule, 
            shaderStages[1]);
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
            SetRasterizationState(rasterization, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
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
    BlitCL::DynamicArray<BlitML::Meshlet>& meshlets, BlitCL::DynamicArray<IndirectOffsets>& indirectDraws)
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
        VkDeviceSize indirectBufferSize = sizeof(IndirectOffsets) * indirectDraws.GetSize();
        // Aside from an indirect buffer, it is also used as a storge buffer as its data will be read and manipulated by compute shaders
        CreateBuffer(m_allocator, m_currentStaticBuffers.drawIndirectBuffer, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, indirectBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        // As explained above this buffer will be accessed by compute shaders through device address
        m_currentStaticBuffers.bufferAddresses.indirectBufferAddress = 
        GetBufferAddress(m_device, m_currentStaticBuffers.drawIndirectBuffer.buffer);

        // The same as the above but this one will be generated by the compute shader each frame after doing frustum culling and other operations
        VkDeviceSize finalIndirectBufferSize = sizeof(IndirectDrawData) * indirectDraws.GetSize();
        CreateBuffer(m_allocator, m_currentStaticBuffers.drawIndirectBufferFinal, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, finalIndirectBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        m_currentStaticBuffers.bufferAddresses.finalIndirectBufferAddress =
        GetBufferAddress(m_device, m_currentStaticBuffers.drawIndirectBufferFinal.buffer);
        
        // This one is a small buffer that will be hold an integer that will be generated by a compute shader and read by draw indirect to decide the number of draw calls
        CreateBuffer(m_allocator, m_currentStaticBuffers.drawIndirectCountBuffer, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT);
        m_currentStaticBuffers.bufferAddresses.indirectCountBufferAddress = 
        GetBufferAddress(m_device, m_currentStaticBuffers.drawIndirectCountBuffer.buffer);

        VkDeviceSize visibilityBufferSize = sizeof(uint32_t) * indirectDraws.GetSize();
        CreateBuffer(m_allocator, m_currentStaticBuffers.drawVisibilityBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY, 
        visibilityBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        m_currentStaticBuffers.bufferAddresses.visibilityBufferAddress =
        GetBufferAddress(m_device, m_currentStaticBuffers.drawVisibilityBuffer.buffer);

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

        if(m_stats.meshShaderSupport)
        {
            CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, 
            m_currentStaticBuffers.globalMeshBuffer.buffer, meshBufferSize, 
            vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize + indirectBufferSize, 0);
        }

        // The visibility buffer will start the 1st frame with only zeroes(nothing will be drawn on the first frame but that is fine)
        vkCmdFillBuffer(m_placeholderCommands, m_currentStaticBuffers.drawVisibilityBuffer.buffer, 0, visibilityBufferSize, 1);
        

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

        // Gets a ref to the frame tools of the current frame
        FrameTools& fTools = m_frameToolsList[m_currentFrame];
        VarBuffers& vBuffers = m_varBuffers[m_currentFrame];

        // Waits for the fence in the current frame tools struct to be signaled and resets it for next time when it gets signalled
        vkWaitForFences(m_device, 1, &(fTools.inFlightFence), VK_TRUE, 1000000000);
        VK_CHECK(vkResetFences(m_device, 1, &(fTools.inFlightFence)))

        m_globalShaderData.projectionView = context.projectionView;
        m_globalShaderData.viewPosition = context.viewPosition;

        m_globalShaderData.sunlightDir = context.sunlightDirection;
        m_globalShaderData.sunlightColor = context.sunlightColor;

        // Create the frustum planes based on the current projection and view matrices 
        m_globalShaderData.frustumData[0] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) + context.projectionTranspose.GetRow(0));
        m_globalShaderData.frustumData[1] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) - context.projectionTranspose.GetRow(0));
        m_globalShaderData.frustumData[2] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) + context.projectionTranspose.GetRow(1));
        m_globalShaderData.frustumData[3] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) - context.projectionTranspose.GetRow(1));
        m_globalShaderData.frustumData[4] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) - context.projectionTranspose.GetRow(2));
        m_globalShaderData.frustumData[5] = BlitML::vec4(0, 0, -1, 100/* Draw distance */);// I need to multiply this with view or projection view
        



        // Asks for the next image in the swapchain to use for presentation, and saves it in swapchainIdx
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_initHandles.swapchain, 1000000000, fTools.imageAcquiredSemaphore, VK_NULL_HANDLE, &swapchainIdx);



        // The data for this descriptor set will be handled in this scope so that it can be accessed by both late and initial render pass
        VkDescriptorSet globalShaderDataSet = VK_NULL_HANDLE;

        // Write the data to the buffer pointers
        *(vBuffers.pGlobalShaderData) = m_globalShaderData;
        *(vBuffers.pBufferAddrs) = m_currentStaticBuffers.bufferAddresses;

        // Write to the shader data binding (binding 0) of the uniform buffer descriptor set
        VkDescriptorBufferInfo globalShaderDataDescriptorBufferInfo{};
        VkWriteDescriptorSet globalShaderDataWrite{};
        WriteBufferDescriptorSets(globalShaderDataWrite, globalShaderDataDescriptorBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        globalShaderDataSet, 0, 1, vBuffers.globalShaderDataBuffer.buffer, 0, VK_WHOLE_SIZE);

        // Write to the buffer address binding (binding 1) of the uniform buffer descriptor set
        VkDescriptorBufferInfo bufferAddressDescriptorBufferInfo{};
        VkWriteDescriptorSet bufferAddressDescriptorWrite{};
        WriteBufferDescriptorSets(bufferAddressDescriptorWrite, bufferAddressDescriptorBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, globalShaderDataSet, 
        1, 1, vBuffers.bufferDeviceAddrsBuffer.buffer, 0, sizeof(BufferDeviceAddresses));

        // Push the descriptor sets to the compute pipelines and the graphics pipelines
        VkWriteDescriptorSet globalShaderDataWrites[2] = {globalShaderDataWrite, bufferAddressDescriptorWrite};

        BeginCommandBuffer(fTools.commandBuffer, 0);



        // Transition the layout of the depth attachment and color attachment to be used as rendering attachments
        {
            VkImageMemoryBarrier2 colorAttachmentBarrier{};
            ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentBarrier, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | 
            VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | 
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, 
            VK_REMAINING_MIP_LEVELS);

            VkImageMemoryBarrier2 depthAttachmentBarrier{};
            ImageMemoryBarrier(m_depthAttachment.image, depthAttachmentBarrier, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_MEMORY_READ_BIT |
            VK_ACCESS_2_MEMORY_WRITE_BIT,VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);

            VkImageMemoryBarrier2 memoryBarriers[2] = {colorAttachmentBarrier, depthAttachmentBarrier};
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, memoryBarriers);
        }
        // Attachments ready for rendering



        // Push the descriptor set for global shader data for the compute and graphics pipelines
        {
            PushDescriptors(m_initHandles.instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_opaqueGraphicsPipelineLayout, 
            0, 2, globalShaderDataWrites);
            PushDescriptors(m_initHandles.instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipelineLayout, 
            0, 2, globalShaderDataWrites);
        }




        /* 
            Dispatching the compute that does frustum culling for objects that were visible last frame
        */
        {
            // Initalize the indirect count buffer as zero
            vkCmdFillBuffer(fTools.commandBuffer, m_currentStaticBuffers.drawIndirectCountBuffer.buffer, 0, sizeof(uint32_t), 0);

            // Before dispatching the compute shader, create a pipeline barrier so that the buffer is filled with 0, before it's processed
            VkBufferMemoryBarrier2 fillBufferBarrier{};
            BufferMemoryBarrier(m_currentStaticBuffers.drawIndirectCountBuffer.buffer, fillBufferBarrier, VK_PIPELINE_STAGE_2_TRANSFER_BIT, 
            VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 
            0, VK_WHOLE_SIZE);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 1, &fillBufferBarrier, 0, nullptr);

            // Bind the shader's pipeline and dispatch the shader to do culling
            vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_indirectCullingComputePipeline);
            vkCmdDispatch(fTools.commandBuffer, static_cast<uint32_t>((context.drawCount / 64) + 1), 1, 1);

            // Stops the indirect stage from reading commands before the compute shader completes
            VkMemoryBarrier2 memoryBarrier{};
            MemoryBarrier(memoryBarrier, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_MEMORY_READ_BIT_KHR);
            PipelineBarrier(fTools.commandBuffer, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
        }




        /* 
            Start the first render pass
        */
        {
            VkRenderingAttachmentInfo colorAttachment{};
            CreateRenderingAttachmentInfo(colorAttachment, m_colorAttachment.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, {0.1f, 0.2f, 0.3f, 0});
            VkRenderingAttachmentInfo depthAttachment{};
            CreateRenderingAttachmentInfo(depthAttachment, m_depthAttachment.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, {0, 0, 0, 0}, {0.f, 0});

            BeginRendering(fTools.commandBuffer, m_drawExtent, {0, 0}, 1, &colorAttachment, &depthAttachment, nullptr);
        }


        // The viewport and scissor are dynamic, so they should be set here
        DefineViewportAndScissor(fTools.commandBuffer, m_drawExtent);



        /*
            Draw the objects that passed frustum culling and were visible last frame 
        */
        {
            // Bind the texture descriptor set. The shader data descriptor set was pushed earlier before the culling shader was dispatched
            vkCmdBindDescriptorSets(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipelineLayout, 1,
            1, &m_currentStaticBuffers.textureDescriptorSet, 0, nullptr);

            // Bind the graphics pipeline and the index buffer
            vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipeline);
            vkCmdBindIndexBuffer(fTools.commandBuffer, m_currentStaticBuffers.globalIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            // Use draw indirect to draw the objects(mesh shading or vertex shader)
            if(m_stats.meshShaderSupport)
            {
               DrawMeshTastsIndirectNv(m_initHandles.instance, fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBuffer.buffer, 
               offsetof(IndirectDrawData, drawIndirectTasks), context.drawCount, sizeof(IndirectDrawData));
            }
            else
            {
                vkCmdDrawIndexedIndirectCount(fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBufferFinal.buffer, 
                offsetof(IndirectDrawData, drawIndirect), m_currentStaticBuffers.drawIndirectCountBuffer.buffer, 0,
                context.drawCount, sizeof(IndirectDrawData));// Ah, the beauty of draw indirect
            }
        }

        // End of first render pass
        vkCmdEndRendering(fTools.commandBuffer);



        VkDescriptorSet depthPyramidImageSet = VK_NULL_HANDLE;
        /*
            Create the depth pyramid before the late render pass, so that it can be used for occlusion culling
        */
        {
            VkImageMemoryBarrier2 shaderDepthBarrier{};
            ImageMemoryBarrier(m_depthAttachment.image, shaderDepthBarrier, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 
            0, VK_REMAINING_MIP_LEVELS);

            VkImageMemoryBarrier2 depthPyramidFirstBarrier{};
            ImageMemoryBarrier(m_depthPyramid.image, depthPyramidFirstBarrier, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

            VkImageMemoryBarrier2 depthPyramidSetBarriers[2] = {shaderDepthBarrier, depthPyramidFirstBarrier};

            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, depthPyramidSetBarriers);

            vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_depthReduceComputePipeline);

            for(size_t i = 0; i < m_depthPyramidMipLevels; ++i)
            {
                VkDescriptorImageInfo sourceImageInfo{};
                sourceImageInfo.imageLayout = (i == 0) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
                sourceImageInfo.imageView = (i == 0) ? m_depthAttachment.imageView : m_depthPyramidMips[i - 1];
                sourceImageInfo.sampler = m_depthAttachmentSampler;

                VkDescriptorImageInfo outImageInfo{};
                outImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                outImageInfo.imageView = m_depthPyramidMips[i];

                VkWriteDescriptorSet write{};
                WriteImageDescriptorSets(write, &sourceImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, depthPyramidImageSet, 1, 1);

                VkWriteDescriptorSet write2{};
                WriteImageDescriptorSets(write2, &outImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, depthPyramidImageSet, 1, 0);

                VkWriteDescriptorSet writes[2] = {write, write2};

                PushDescriptors(m_initHandles.instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_depthReducePipelineLayout, 
                0, 2, writes);

                uint32_t levelWidth = BlitML::Max(1u, (m_depthPyramidExtent.width) >> i);
                uint32_t levelHeight = BlitML::Max(1u, (m_depthPyramidExtent.height) >> i);

                ShaderPushConstant pushConstant;
                pushConstant.imageSize = {static_cast<float>(levelWidth), static_cast<float>(levelHeight)};
                vkCmdPushConstants(fTools.commandBuffer, m_depthReducePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ShaderPushConstant), 
                &pushConstant);

                vkCmdDispatch(fTools.commandBuffer, levelWidth / 32 + 1, levelHeight / 32 + 1, 1);

                VkImageMemoryBarrier2 dispatchWriteBarrier{};
                ImageMemoryBarrier(m_depthPyramid.image, dispatchWriteBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, 
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
                PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &dispatchWriteBarrier);
            }

            VkImageMemoryBarrier2 depthPyramidCompleteBarrier{};
            ImageMemoryBarrier(m_depthAttachment.image, depthPyramidCompleteBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
            VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
            | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &depthPyramidCompleteBarrier);
        }


        VkDescriptorBufferInfo cullingDataBufferInfo{};
        VkWriteDescriptorSet cullingDataWrite{};
        WriteBufferDescriptorSets(cullingDataWrite, cullingDataBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, globalShaderDataSet, 
        2, 1, vBuffers.cullingDataBuffer.buffer, 0, VK_WHOLE_SIZE);

        VkDescriptorImageInfo depthPyramidImageInfo{};
        depthPyramidImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        depthPyramidImageInfo.imageView = m_depthAttachment.imageView;
        depthPyramidImageInfo.sampler = m_depthAttachmentSampler;
        VkWriteDescriptorSet depthPyramidWrite{};
        WriteImageDescriptorSets(depthPyramidWrite, &depthPyramidImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, globalShaderDataSet, 
        1, 3);

        VkWriteDescriptorSet latePassGlobalShaderDataSet[4] = {globalShaderDataWrite, bufferAddressDescriptorWrite, 
        cullingDataWrite, depthPyramidWrite};

        CullingData cullingData;
        cullingData.proj0 = context.projectionView[0];
        cullingData.proj11 = context.projectionView[11];
        cullingData.zNear = 1.f;
        cullingData.pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
        cullingData.pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);
        *(vBuffers.pCullingData) = cullingData;
        
        // Push the descriptor set for global shader data for the compute and graphics pipelines
        {
            PushDescriptors(m_initHandles.instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_lateCullingPipelineLayout, 
            0, 4, latePassGlobalShaderDataSet);
            PushDescriptors(m_initHandles.instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipelineLayout, 
            0, 2, globalShaderDataWrites);
        }



        /*
            Execute culling tests for late render pass
        */
        {
            // Wait for draw indirect to finish before filling the indirect count buffer with zeroes
            VkBufferMemoryBarrier2 waitForDrawIndirectCount{};
            BufferMemoryBarrier(m_currentStaticBuffers.drawIndirectCountBuffer.buffer, waitForDrawIndirectCount, VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, 
            VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 1, &waitForDrawIndirectCount, 0, nullptr);

            // Fill the indirect count buffer with 0
            vkCmdFillBuffer(fTools.commandBuffer, m_currentStaticBuffers.drawIndirectCountBuffer.buffer, 0, sizeof(uint32_t), 0);

            // Wait for the transfer operation to finish before dispatching the compute shader
            VkBufferMemoryBarrier2 waitForBufferFill{};
            BufferMemoryBarrier(m_currentStaticBuffers.drawIndirectCountBuffer.buffer, waitForBufferFill, VK_PIPELINE_STAGE_2_TRANSFER_BIT, 
            VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT, 
            0, VK_WHOLE_SIZE);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 1, &waitForBufferFill, 0, nullptr);

            // Dispatch the compute shader
            vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_lateCullingComputePipeline);
            vkCmdDispatch(fTools.commandBuffer, static_cast<uint32_t>(context.drawCount / 64) + 1, 1, 1);

            // Stops the indirect stage from reading commands before the compute shader completes
            VkMemoryBarrier2 memoryBarrier{};
            MemoryBarrier(memoryBarrier, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_MEMORY_READ_BIT_KHR);
            PipelineBarrier(fTools.commandBuffer, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
        }



        /* 
            Start the later Render Pass
        */
        {
            VkRenderingAttachmentInfo colorAttachment{};
            CreateRenderingAttachmentInfo(colorAttachment, m_colorAttachment.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
            VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
            VkRenderingAttachmentInfo depthAttachment{};
            CreateRenderingAttachmentInfo(depthAttachment, m_depthAttachment.imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
            VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_DONT_CARE);
            BeginRendering(fTools.commandBuffer, m_drawExtent, {0, 0}, 1, &colorAttachment, &depthAttachment, nullptr);
        }



        /*
            Draw the objects that passed frustum culling and were not visible last frame 
        */
        {
            // Bind the texture descriptor set. The shader data descriptor set was pushed earlier before the culling shader was dispatched
            vkCmdBindDescriptorSets(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipelineLayout, 1,
            1, &m_currentStaticBuffers.textureDescriptorSet, 0, nullptr);

            // Bind the graphics pipeline and the index buffer
            vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipeline);
            vkCmdBindIndexBuffer(fTools.commandBuffer, m_currentStaticBuffers.globalIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            // Use draw indirect to draw the objects(mesh shading or vertex shader)
            if(m_stats.meshShaderSupport)
            {
               DrawMeshTastsIndirectNv(m_initHandles.instance, fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBuffer.buffer, 
               offsetof(IndirectDrawData, drawIndirectTasks), context.drawCount, sizeof(IndirectDrawData));
            }
            else
            {
                vkCmdDrawIndexedIndirectCount(fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBufferFinal.buffer, 
                offsetof(IndirectDrawData, drawIndirect), m_currentStaticBuffers.drawIndirectCountBuffer.buffer, 0,
                context.drawCount, sizeof(IndirectDrawData));// Ah, the beauty of draw indirect
            }
        }

        // End of late render pass
        vkCmdEndRendering(fTools.commandBuffer);




        // Copying the color attachment to the swapchain image and transitioning the image to present
        {
            // color attachment barrier
            VkImageMemoryBarrier2 colorAttachmentBarrier{};
            ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentBarrier, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | 
            VK_ACCESS_2_MEMORY_WRITE_BIT, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

            //swapchain image barrier
            VkImageMemoryBarrier2 swapchainImageBarrier{};
            ImageMemoryBarrier(m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], swapchainImageBarrier, VK_PIPELINE_STAGE_2_NONE, 
            VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

            VkImageMemoryBarrier2 firstTransferMemoryBarriers[2] = {colorAttachmentBarrier, swapchainImageBarrier};

            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, firstTransferMemoryBarriers);

            if(context.debugPyramid)
            {
                uint32_t debugLevel = 0;
                VkImageSubresourceLayers colorAttachmentSL{};
                colorAttachmentSL.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                colorAttachmentSL.baseArrayLayer = 0;
                colorAttachmentSL.layerCount = 1;
                colorAttachmentSL.mipLevel = debugLevel;
                VkImageSubresourceLayers swapchainImageSL{};
                swapchainImageSL.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                swapchainImageSL.baseArrayLayer = 0;
                swapchainImageSL.layerCount = 1;
                swapchainImageSL.mipLevel = 0;
                CopyImageToImage(fTools.commandBuffer, m_depthPyramid.image, VK_IMAGE_LAYOUT_GENERAL, 
                m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                {uint32_t(BlitML::Max(1u, (m_depthPyramidExtent.width) >> debugLevel)), uint32_t(BlitML::Max(1u, (m_depthPyramidExtent.height) >> debugLevel))}, 
                m_initHandles.swapchainExtent, colorAttachmentSL, swapchainImageSL, VK_FILTER_NEAREST);
            }
            else
            {
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
                m_initHandles.swapchainExtent, colorAttachmentSL, swapchainImageSL, VK_FILTER_LINEAR);
            }

            // Change the swapchain image layout to present
            VkImageMemoryBarrier2 presentImageBarrier{};
            ImageMemoryBarrier(m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], presentImageBarrier, VK_PIPELINE_STAGE_2_BLIT_BIT, 
            VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
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