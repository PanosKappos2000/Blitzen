#include "vulkanRenderer.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

void DrawMeshTasks(VkInstance instance, VkCommandBuffer commandBuffer, VkBuffer drawBuffer, 
VkDeviceSize drawOffset, VkBuffer countBuffer, VkDeviceSize countOffset, uint32_t maxDrawCount, uint32_t stride) 
{
    auto func = (PFN_vkCmdDrawMeshTasksIndirectCountEXT) vkGetInstanceProcAddr(instance, "vkCmdDrawMeshTasksIndirectCountEXT");
    if (func != nullptr) 
    {
        func(commandBuffer, drawBuffer, drawOffset, countBuffer, countOffset, maxDrawCount, stride);
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

    void VulkanRenderer::CreateDescriptorLayouts()
    {
        // Binding used by both compute and graphics pipelines, to access global data like the view matrix
        VkDescriptorSetLayoutBinding shaderDataLayoutBinding{};
        // Binding used by both compute and graphics pipelines, to access the addresses of storage buffers
        VkDescriptorSetLayoutBinding bufferAddressBinding{};

        // If mesh shaders are used the bindings needs to be accessed by mesh shaders, otherwise they will be accessed by vertex shader stage
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

        // Binding for the global shader data layout, used by culling shaders to do occlusion and frustum culling 
        VkDescriptorSetLayoutBinding cullingDataLayoutBinding{};
        CreateDescriptorSetLayoutBinding(cullingDataLayoutBinding, 2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
        VK_SHADER_STAGE_COMPUTE_BIT);

        // Binding for the global shader data layout, used by culling shaders to access the depth pyramid
        VkDescriptorSetLayoutBinding depthImageBinding{};
        CreateDescriptorSetLayoutBinding(depthImageBinding, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
        VK_SHADER_STAGE_COMPUTE_BIT);
        
        // All bindings combined to create the global shader data descriptor set layout
        VkDescriptorSetLayoutBinding shaderDataBindings[4] = {shaderDataLayoutBinding, bufferAddressBinding, 
        cullingDataLayoutBinding, depthImageBinding};

        m_pushDescriptorBufferLayout = CreateDescriptorSetLayout(m_device, 4, shaderDataBindings, 
        VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);




        // Descriptor set layout for textures
        VkDescriptorSetLayoutBinding texturesLayoutBinding{};
        CreateDescriptorSetLayoutBinding(texturesLayoutBinding, 0, static_cast<uint32_t>(m_currentStaticBuffers.loadedTextures.GetSize()), 
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_currentStaticBuffers.textureDescriptorSetlayout = CreateDescriptorSetLayout(m_device, 1, &texturesLayoutBinding);




        // Binding for input image in depth pyramid creation shader
        VkDescriptorSetLayoutBinding inImageLayoutBinding{};
        CreateDescriptorSetLayoutBinding(inImageLayoutBinding, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

        // Bindng for output image in depth pyramid creation shader
        VkDescriptorSetLayoutBinding outImageLayoutBinding{};
        CreateDescriptorSetLayoutBinding(outImageLayoutBinding, 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

        // Combine the bindings for the final descriptor layout
        VkDescriptorSetLayoutBinding storageImageBindings[2] = {inImageLayoutBinding, outImageLayoutBinding};
        m_depthPyramidDescriptorLayout = CreateDescriptorSetLayout(m_device, 2, storageImageBindings, 
        VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
    }

    void VulkanRenderer::SetupForRendering(GPUData& gpuData)
    {
        BLIT_ASSERT_MESSAGE(gpuData.drawCount, "Nothing to draw")
        BLIT_ASSERT(gpuData.drawCount <= BLITZEN_VULKAN_MAX_DRAW_CALLS)

        VarBuffersInit();

        // Setting the size of the texture array here , so that the pipeline knows how many descriptors the layout should have
        m_currentStaticBuffers.loadedTextures.Resize(gpuData.textureCount);

        CreateDescriptorLayouts();

        // Pipeline layouts for all shaders
        {
            VkDescriptorSetLayout layouts [2] = {m_pushDescriptorBufferLayout, m_currentStaticBuffers.textureDescriptorSetlayout};

            CreatePipelineLayout(m_device, &m_opaqueGraphicsPipelineLayout, 2, layouts, 0, nullptr);

            // This is the layout for the culling shaders
            CreatePipelineLayout(m_device, &m_lateCullingPipelineLayout, 1, &m_pushDescriptorBufferLayout, 0, nullptr);

            VkPushConstantRange pushConstant{};
            CreatePushConstantRange(pushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(ShaderPushConstant));
            CreatePipelineLayout(m_device, &m_depthReducePipelineLayout, 1, &m_depthPyramidDescriptorLayout, 1, &pushConstant);
        }



        // Texture sampler, for now all textures will use the same one
        CreateTextureSampler(m_device, m_placeholderSampler);

        size_t objectId = 0;
        gpuData.gameObjects.Resize(gpuData.drawCount);// This is temporary for testing. Game objects will not be created in the renderer
        BlitCL::DynamicArray<RenderObject> renderObjects(gpuData.drawCount);
        BlitCL::DynamicArray<MeshInstance> instances(gpuData.gameObjects.GetSize());
        {
            // Load multiple bunny mesh instances
            for(size_t i = 0; i < gpuData.gameObjects.GetSize() - gpuData.gameObjects.GetSize() / 10; ++i)
            {
                MeshInstance& instance = instances[i];

                // Loading random position and scale. Normally you would get this from the game object
                BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1000 - 50,//x 
                (float(rand()) / RAND_MAX) * 1000 - 50,//y
                (float(rand()) / RAND_MAX) * 1000 - 50);//z
                instance.pos = translation;
                instance.scale = 5.f;

                // Loading random orientation. Normally you would get this from the game object
                BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
		        float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
                BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
                instance.orientation = orientation;

                // Normally every game object would have an index to the mesh that it is using. For now, the bunny mesh is loaded manually
                BlitzenEngine::MeshAssets& currentMesh = gpuData.pMeshes[/*currentObject.meshIndex*/1];

                for(size_t j = 0; j < currentMesh.surfaces.GetSize(); ++j)
                {
                    RenderObject& currentObject = renderObjects[objectId];

                    //currentObject.surfaceId = currentMesh.surfaces[j].surfaceId;
                    /* Every surface holds its own Id, 
                    unforunately this cannot work with how I have setup asset loading at the moment, but I will fix it in the near future
                    For now I will hard code it*/
                    currentObject.surfaceId = 1;
                    currentObject.meshInstanceId = i;// Each game object has one mesh, so the mesh instance id is equal to the game object id

                    objectId++;
                }
            }

            // Load multiple kitten mesh instances
            for (size_t i = gpuData.gameObjects.GetSize() - gpuData.gameObjects.GetSize() / 10; i < gpuData.drawCount; ++i)
            {
                MeshInstance& instance = instances[i];

                // Loading random position and scale. Normally you would get this from the game object
                BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1000 - 50,//x 
                (float(rand()) / RAND_MAX) * 1000 - 50,//y
                (float(rand()) / RAND_MAX) * 1000 - 50);//z
                instance.pos = translation;
                instance.scale = 1.f;

                // Loading random orientation. Normally you would get this from the game object
                BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
		        float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
                BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
                instance.orientation = orientation;

                // Normally every game object would have an index to the mesh that it is using. For now, the kitten mesh is loaded manually
                BlitzenEngine::MeshAssets& currentMesh = gpuData.pMeshes[/*currentObject.meshIndex*/0];

                for(size_t j = 0; j < currentMesh.surfaces.GetSize(); ++j)
                {
                    RenderObject& currentObject = renderObjects[objectId];

                    //currentRo.surfaceId = currentMesh.surfaces[j].surfaceId;
                    /* Every surface holds its own Id, 
                    unforunately this cannot work with how I have setup asset loading at the moment, but I will fix it in the near future
                    For now I will hard code it*/
                    currentObject.surfaceId = 0;
                    currentObject.meshInstanceId = i;// Each game object has one mesh, so the mesh instance id is equal to the game object id

                    objectId++;
                }
            }
        }

        // Pass each surface from each mesh to a separate array. This should be done earlier by having each mesh index into its surfaces
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface> surfaces;
        surfaces.Reserve(gpuData.meshCount * 10);// reserve some space too avoid too many reallocations
        size_t surfaceIndex = 0;
        for(size_t i = 0; i < gpuData.meshCount; ++i)
        {
            BlitzenEngine::MeshAssets& currentMesh = gpuData.pMeshes[i];
            surfaces.Resize(surfaces.GetSize() + currentMesh.surfaces.GetSize());
            for(size_t j = 0; j < currentMesh.surfaces.GetSize(); ++j)
            {
                surfaces[surfaceIndex] = currentMesh.surfaces[j];
                surfaceIndex++;
            }
        }

        // Allocating an image for each texture and telling it to use the one sampler the renderer creates (for now)
        for(size_t i = 0; i < gpuData.textureCount; ++i)
        {
            
            CreateTextureImage(reinterpret_cast<void*>(gpuData.pTextures[i].pTextureData), m_device, m_allocator, 
            m_currentStaticBuffers.loadedTextures[i].image, 
            {(uint32_t)gpuData.pTextures[i].textureWidth, (uint32_t)gpuData.pTextures[i].textureHeight, 1}, VK_FORMAT_R8G8B8A8_UNORM, 
            VK_IMAGE_USAGE_SAMPLED_BIT, m_placeholderCommands, m_graphicsQueue.handle, 0);

            m_currentStaticBuffers.loadedTextures[i].sampler = m_placeholderSampler;
        }

        // Configure the material data to what is actually needed by the GPU
        BlitCL::DynamicArray<MaterialConstants> materials(gpuData.materialCount);
        for(size_t i = 0; i < gpuData.materialCount; ++i)
        {
            materials[i].diffuseColor = gpuData.pMaterials[i].diffuseColor;
            materials[i].diffuseTextureTag = gpuData.pMaterials[i].diffuseMapTag;
        }

        // Passes the data needed to perform bindless rendering (uploads it to storage buffers)
        UploadDataToGPU(gpuData.vertices, gpuData.indices, renderObjects, materials, gpuData.meshlets, surfaces, instances);
    
        // First render pass Culling compute shader
        CreateComputeShaderProgram(m_device, "VulkanShaders/IndirectCulling.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
        m_lateCullingPipelineLayout, &m_indirectCullingComputePipeline);
        
        // Depth pyramid generation compute shader
        CreateComputeShaderProgram(m_device, "VulkanShaders/DepthReduce.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
        m_depthReducePipelineLayout, &m_depthReduceComputePipeline);
        
        // Later render pass culling compute shader
        CreateComputeShaderProgram(m_device, "VulkanShaders/LateCulling.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
        m_lateCullingPipelineLayout, &m_lateCullingComputePipeline);
        
        // Graphics pipeline setup and vertex / mesh shader and fragment shader loading
        SetupMainGraphicsPipeline();
    }

    void VulkanRenderer::UploadDataToGPU(BlitCL::DynamicArray<BlitML::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices, 
    BlitCL::DynamicArray<RenderObject>& renderObjects, BlitCL::DynamicArray<MaterialConstants>& materials, 
    BlitCL::DynamicArray<BlitML::Meshlet>& meshlets, BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces, 
    BlitCL::DynamicArray<MeshInstance>& meshInstances)
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

        // The render object buffer will an index to mesh instance data and one to surface data
        VkDeviceSize renderObjectBufferSize = sizeof(RenderObject) * renderObjects.GetSize();
        CreateBuffer(m_allocator, m_currentStaticBuffers.renderObjectBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, sizeof(RenderObject) * BLITZEN_VULKAN_MAX_DRAW_CALLS, VMA_ALLOCATION_CREATE_MAPPED_BIT);
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

        // Holds the data to draw any surface that was loaded
        VkDeviceSize surfaceBufferSize = sizeof(BlitzenEngine::PrimitiveSurface) * surfaces.GetSize();
        CreateBuffer(m_allocator, m_currentStaticBuffers.surfaceBuffer, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, surfaceBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        // Will be accessed in compute and vertex shaders
        m_currentStaticBuffers.bufferAddresses.surfaceBufferAddress = 
        GetBufferAddress(m_device, m_currentStaticBuffers.surfaceBuffer.buffer);

        VkDeviceSize meshInstanceBufferSize = sizeof(MeshInstance) * meshInstances.GetSize();
        CreateBuffer(m_allocator, m_currentStaticBuffers.meshInstanceBuffer, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VMA_MEMORY_USAGE_GPU_ONLY, meshInstanceBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        // Will be accessed in compute and vertex shaders
        m_currentStaticBuffers.bufferAddresses.meshInstanceBufferAddress = 
        GetBufferAddress(m_device, m_currentStaticBuffers.meshInstanceBuffer.buffer);

        // This is the buffer that will take the data from the surfaces to draw every object in the scene. Filled in the culling shaders
        VkDeviceSize finalIndirectBufferSize = sizeof(IndirectDrawData) * renderObjects.GetSize();
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

        // Holds a value that represents if an object was visible last frame or not. Filled by late culling shader
        VkDeviceSize visibilityBufferSize = sizeof(uint32_t) * renderObjects.GetSize();
        CreateBuffer(m_allocator, m_currentStaticBuffers.drawVisibilityBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_GPU_ONLY, 
        visibilityBufferSize, VMA_ALLOCATION_CREATE_MAPPED_BIT);
        m_currentStaticBuffers.bufferAddresses.visibilityBufferAddress =
        GetBufferAddress(m_device, m_currentStaticBuffers.drawVisibilityBuffer.buffer);

        // This holds the combined size of all the required buffers to pass to the combined staging buffer
        VkDeviceSize stagingBufferSize = vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize + surfaceBufferSize + 
        meshInstanceBufferSize;

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
        BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize + indexBufferSize, renderObjects.Data(), 
        renderObjectBufferSize);
        BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize + indexBufferSize + renderObjectBufferSize, 
        materials.Data(), materialBufferSize);
        BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize, 
        surfaces.Data(), surfaceBufferSize);
        BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize +
        surfaceBufferSize, meshInstances.Data(), meshInstanceBufferSize);

        if(m_stats.meshShaderSupport)
        {
            BlitzenCore::BlitMemCopy(reinterpret_cast<uint8_t*>(pData) + vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize
            + surfaceBufferSize + meshInstanceBufferSize, meshlets.Data(), meshBufferSize);
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
        m_currentStaticBuffers.surfaceBuffer.buffer, surfaceBufferSize, 
        vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize, 0);

        CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer,
        m_currentStaticBuffers.meshInstanceBuffer.buffer, meshInstanceBufferSize, 
        vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize + surfaceBufferSize, 0);

        if(m_stats.meshShaderSupport)
        {
            CopyBufferToBuffer(m_placeholderCommands, stagingBuffer.buffer, 
            m_currentStaticBuffers.globalMeshBuffer.buffer, meshBufferSize, 
            vertexBufferSize + indexBufferSize + renderObjectBufferSize + materialBufferSize + surfaceBufferSize, 0);
        }

        // The visibility buffer will start the 1st frame with only zeroes(nothing will be drawn on the first frame but that is fine)
        vkCmdFillBuffer(m_placeholderCommands, m_currentStaticBuffers.drawVisibilityBuffer.buffer, 0, visibilityBufferSize, 0);
        

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

        // Projection and view coordinate parameters will be passed to the global shader data buffer
        m_globalShaderData.projectionView = context.projectionView;
        m_globalShaderData.viewPosition = context.viewPosition;
        m_globalShaderData.view = context.viewMatrix;

        // Global lighting parameters will be written to the global shader data buffer
        m_globalShaderData.sunlightDir = context.sunlightDirection;
        m_globalShaderData.sunlightColor = context.sunlightColor;

        // Culling data will be written to the culling data buffer
        CullingData cullingData;

        // Create the frustum planes based on the current projection matrix, will be written to the culling data buffer
        cullingData.frustumData[0] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) + context.projectionTranspose.GetRow(0));
        cullingData.frustumData[1] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) - context.projectionTranspose.GetRow(0));
        cullingData.frustumData[2] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) + context.projectionTranspose.GetRow(1));
        cullingData.frustumData[3] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) - context.projectionTranspose.GetRow(1));
        cullingData.frustumData[4] = BlitML::NormalizePlane(context.projectionTranspose.GetRow(3) - context.projectionTranspose.GetRow(2));
        cullingData.frustumData[5] = BlitML::vec4(0, 0, -1, context.drawDistance);

        // Culling data for occlusion culling
        cullingData.proj0 = context.projectionMatrix[0];
        cullingData.proj5 = context.projectionMatrix[5];
        cullingData.zNear = context.zNear;
        cullingData.pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
        cullingData.pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);

        // Debug values to deactivate occlusion culling and other algorithms
        cullingData.occlusionEnabled = context.occlusionEnabled;
        cullingData.lodEnabled = context.lodEnabled;

        // Write the data to the buffer pointers
        *(vBuffers.pGlobalShaderData) = m_globalShaderData;
        *(vBuffers.pBufferAddrs) = m_currentStaticBuffers.bufferAddresses;
        *(vBuffers.pCullingData) = cullingData;
        



        // Asks for the next image in the swapchain to use for presentation, and saves it in swapchainIdx
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_initHandles.swapchain, 1000000000, fTools.imageAcquiredSemaphore, VK_NULL_HANDLE, &swapchainIdx);



        // The data for this descriptor set will be handled in this scope so that it can be accessed by both late and initial render pass
        VkDescriptorSet globalShaderDataSet = VK_NULL_HANDLE;

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

        VkDescriptorBufferInfo cullingDataBufferInfo{};
        VkWriteDescriptorSet cullingDataWrite{};
        WriteBufferDescriptorSets(cullingDataWrite, cullingDataBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, globalShaderDataSet, 
        2, 1, vBuffers.cullingDataBuffer.buffer, 0, VK_WHOLE_SIZE);

        // Push the descriptor sets to the compute pipelines and the graphics pipelines
        VkWriteDescriptorSet globalShaderDataWrites[3] = {globalShaderDataWrite, bufferAddressDescriptorWrite, cullingDataWrite};

        BeginCommandBuffer(fTools.commandBuffer, 0);



       
        // Push the descriptor set for global shader data for the compute and graphics pipelines
        {
            PushDescriptors(m_initHandles.instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_lateCullingPipelineLayout, 
            0, 3, globalShaderDataWrites);
            PushDescriptors(m_initHandles.instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGraphicsPipelineLayout, 
            0, 2, globalShaderDataWrites);
        }

        /* 
            Dispatching the compute that does frustum culling for objects that were visible last frame
        */
        {
            // Wait for previous frame later pass to be done with the indirect count buffer before zeroing it
            VkBufferMemoryBarrier2 waitBeforeZeroingCountBuffer{};
            BufferMemoryBarrier(m_currentStaticBuffers.drawIndirectCountBuffer.buffer, waitBeforeZeroingCountBuffer,
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 1, &waitBeforeZeroingCountBuffer, 0, nullptr);

            // Initialize the indirect count buffer as zero
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

            // Stops the indirect stage from reading commands until the compute shader completes
            VkMemoryBarrier2 memoryBarrier{};
            MemoryBarrier(memoryBarrier, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_MEMORY_READ_BIT_KHR);
            PipelineBarrier(fTools.commandBuffer, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
        }




        /*
            Tranition the image layout of the color and depth attachments to be used as rendering attachments
        */
        {
            VkImageMemoryBarrier2 colorAttachmentBarrier{};
            ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentBarrier, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0,
            VK_REMAINING_MIP_LEVELS);

            VkImageMemoryBarrier2 depthAttachmentBarrier{};
            ImageMemoryBarrier(m_depthAttachment.image, depthAttachmentBarrier, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);

            VkImageMemoryBarrier2 memoryBarriers[2] = { colorAttachmentBarrier, depthAttachmentBarrier };
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, memoryBarriers);
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
               DrawMeshTasks(m_initHandles.instance, fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBufferFinal.buffer, 
               offsetof(IndirectDrawData, drawIndirectTasks), m_currentStaticBuffers.drawIndirectCountBuffer.buffer, 0, 
               0, sizeof(IndirectDrawData));
            }
            else
            {
                vkCmdDrawIndexedIndirectCount(fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBufferFinal.buffer, 
                offsetof(IndirectDrawData, drawIndirect), m_currentStaticBuffers.drawIndirectCountBuffer.buffer, 0,
                static_cast<uint32_t>(context.drawCount), sizeof(IndirectDrawData));// Ah, the beauty of draw indirect
            }
        }

        // End of first render pass
        vkCmdEndRendering(fTools.commandBuffer);



        VkDescriptorSet depthPyramidImageSet = VK_NULL_HANDLE;
        /*
            Create the depth pyramid before the late render pass, so that it can be used for occlusion culling
        */
        {
            // Wait for the previous render pass to be done with the depth attachment
            VkImageMemoryBarrier2 shaderDepthBarrier{};
            ImageMemoryBarrier(m_depthAttachment.image, shaderDepthBarrier, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 
            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 
            0, VK_REMAINING_MIP_LEVELS);

            // Wait for the previous frame to be done with the depth pyramid 
            VkImageMemoryBarrier2 depthPyramidFirstBarrier{};
            ImageMemoryBarrier(m_depthPyramid.image, depthPyramidFirstBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 
            VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

            VkImageMemoryBarrier2 depthPyramidSetBarriers[2] = {shaderDepthBarrier, depthPyramidFirstBarrier};
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, depthPyramidSetBarriers);

            // Bind the compute pipeline, the depth pyramid will be called for as many depth pyramid mip levels there are
            vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_depthReduceComputePipeline);

            // Call the depth pyramid creation compute shader for every mip level in the depth pyramid
            for(size_t i = 0; i < m_depthPyramidMipLevels; ++i)
            {
                // Pass the depth attachment image view or the previous image view of the depth pyramid as the image to be read by the shader
                VkDescriptorImageInfo sourceImageInfo{};
                VkWriteDescriptorSet write{};
                WriteImageDescriptorSets(write, sourceImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, depthPyramidImageSet, 1, 
                (i == 0) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL, 
                (i == 0) ? m_depthAttachment.imageView : m_depthPyramidMips[i - 1], m_depthAttachmentSampler);

                // Pass the current depth pyramid image view to the shader as the output
                VkDescriptorImageInfo outImageInfo{};
                VkWriteDescriptorSet write2{};
                WriteImageDescriptorSets(write2, outImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, depthPyramidImageSet, 0, 
                VK_IMAGE_LAYOUT_GENERAL, m_depthPyramidMips[i]);
                VkWriteDescriptorSet writes[2] = {write, write2};

                // Push the descriptor sets
                PushDescriptors(m_initHandles.instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_depthReducePipelineLayout, 
                0, 2, writes);

                // Calculate the extent of the current depth pyramid mip level
                uint32_t levelWidth = BlitML::Max(1u, (m_depthPyramidExtent.width) >> i);
                uint32_t levelHeight = BlitML::Max(1u, (m_depthPyramidExtent.height) >> i);
                // Pass the extent to the push constant
                ShaderPushConstant pushConstant;
                pushConstant.imageSize = {static_cast<float>(levelWidth), static_cast<float>(levelHeight)};
                vkCmdPushConstants(fTools.commandBuffer, m_depthReducePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ShaderPushConstant), 
                &pushConstant);

                // Dispatch the shader
                vkCmdDispatch(fTools.commandBuffer, levelWidth / 32 + 1, levelHeight / 32 + 1, 1);

                // Wait for the shader to finish before the next loop calls it again
                VkImageMemoryBarrier2 dispatchWriteBarrier{};
                ImageMemoryBarrier(m_depthPyramid.image, dispatchWriteBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, 
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
                PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &dispatchWriteBarrier);
            }
        }

        // The later culling compute shader needs the depth pyramid descriptor on top of everything else that the first version gets
        VkDescriptorImageInfo depthPyramidImageInfo{};
        VkWriteDescriptorSet depthPyramidWrite{};
        WriteImageDescriptorSets(depthPyramidWrite, depthPyramidImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, globalShaderDataSet, 
        3, VK_IMAGE_LAYOUT_GENERAL, m_depthPyramid.imageView, m_depthAttachmentSampler);

        VkWriteDescriptorSet latePassGlobalShaderDataSet[4] = {globalShaderDataWrite, bufferAddressDescriptorWrite, 
        cullingDataWrite, depthPyramidWrite};
        
        // Pass the push descriptors to the culling pipeline and the graphics pipeline
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

        // Once the compute shader is done, the depth attachment can transition to depth attachment layout
        VkImageMemoryBarrier2 depthAttachmentReadBarrier{};
        ImageMemoryBarrier(m_depthAttachment.image, depthAttachmentReadBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
        | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &depthAttachmentReadBarrier);

        /* 
            Start the later Render Pass
        */
        {
            VkRenderingAttachmentInfo colorAttachment{};
            CreateRenderingAttachmentInfo(colorAttachment, m_colorAttachment.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
            VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE, {0.1f, 0.2f, 0.3f, 0});
            VkRenderingAttachmentInfo depthAttachment{};
            CreateRenderingAttachmentInfo(depthAttachment, m_depthAttachment.imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
            VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);
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
               /*DrawMeshTastsIndirectNv(m_initHandles.instance, fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBuffer.buffer, 
               offsetof(IndirectDrawData, drawIndirectTasks), static_cast<uint32_t>(context.drawCount), sizeof(IndirectDrawData));
               With how things have changed I can't really use the mesh shaders for the time being*/
            }
            else
            {
                vkCmdDrawIndexedIndirectCount(fTools.commandBuffer, m_currentStaticBuffers.drawIndirectBufferFinal.buffer, 
                offsetof(IndirectDrawData, drawIndirect), m_currentStaticBuffers.drawIndirectCountBuffer.buffer, 0,
                static_cast<uint32_t>(context.drawCount), sizeof(IndirectDrawData));
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
        // Create a new swapchain by passing an empty swapchain handle to the new swapchain argument and the old swapchain to oldSwapchain
        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        CreateSwapchain(m_device, m_initHandles, windowWidth, windowHeight, m_graphicsQueue, 
        m_presentQueue, m_computeQueue, m_pCustomAllocator, newSwapchain, m_initHandles.swapchain);

        // The draw extent should also be updated depending on if the swapchain got bigger or smaller
        m_drawExtent.width = std::min(windowWidth, m_drawExtent.width);
        m_drawExtent.height = std::min(windowHeight, m_drawExtent.height);
        // TODO: Recreate the depth pyramid if the the draw extent changes

        // Wait for the GPU to be done with the swapchain and destroy the swapchain
        vkDeviceWaitIdle(m_device);
        vkDestroySwapchainKHR(m_device, m_initHandles.swapchain, nullptr);
        m_initHandles.swapchain = newSwapchain;
    }
}