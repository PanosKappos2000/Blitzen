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
    uint8_t VulkanRenderer::VarBuffersInit()
    {
        for(size_t i = 0; i < BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VarBuffers& buffers = m_varBuffers[i];

            // Tries to create the global shader data uniform buffer
            if(!CreateBuffer(m_allocator, buffers.globalShaderDataBuffer.buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
            sizeof(BlitzenEngine::GlobalShaderData), VMA_ALLOCATION_CREATE_MAPPED_BIT))
                return 0;
            // If everything went fine, get the persistent mapped pointer to the buffer
            buffers.globalShaderDataBuffer.pData = reinterpret_cast<BlitzenEngine::GlobalShaderData*>(
            buffers.globalShaderDataBuffer.buffer.allocation->GetMappedData());
            // Creates the VkWriteDescriptor for this buffer here, as it will reamain constant 
            WriteBufferDescriptorSets(buffers.globalShaderDataBuffer.descriptorWrite, buffers.globalShaderDataBuffer.bufferInfo, 
            buffers.globalShaderDataBuffer.descriptorType, buffers.globalShaderDataBuffer.descriptorBinding, 
            buffers.globalShaderDataBuffer.buffer.buffer);

            // Tries create the buffer address uniform buffer
            if(!CreateBuffer(m_allocator, buffers.cullingDataBuffer.buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
            sizeof(BlitzenEngine::CullingData), VMA_ALLOCATION_CREATE_MAPPED_BIT))
                return 0;
            // If everything went fine, get the persistent mapped pointer to the buffer
            buffers.cullingDataBuffer.pData = reinterpret_cast<BlitzenEngine::CullingData*>(
            buffers.cullingDataBuffer.buffer.allocation->GetMappedData());
            // Creates the VkWriteDescriptor for this buffer here, as it will remain constant
            WriteBufferDescriptorSets(buffers.cullingDataBuffer.descriptorWrite, buffers.cullingDataBuffer.bufferInfo, 
            buffers.cullingDataBuffer.descriptorType, buffers.cullingDataBuffer.descriptorBinding, 
            buffers.cullingDataBuffer.buffer.buffer);
        }

        return 1;
    }

    uint8_t VulkanRenderer::CreateDescriptorLayouts()
    {
        // Binding used by both compute and graphics pipelines, to access global data like the view matrix
        VkDescriptorSetLayoutBinding shaderDataLayoutBinding{};
        // Binding used by both compute and graphics pipelines, to access the vertex buffer
        VkDescriptorSetLayoutBinding vertexBufferBinding{};
        // Binding used for indirect commands in mesh shaders
        VkDescriptorSetLayoutBinding indirectTaskBufferBinding{};
        // Binding used for surface storage buffer, accessed by either mesh or vertex shader and compute
        VkDescriptorSetLayoutBinding surfaceBufferBinding{};
        // Binding used for clusters. Cluster can be used by either mesh or vertex shader and compute shaders
        VkDescriptorSetLayoutBinding meshletBufferBinding{};
        // Binding used for meshlet indices
        VkDescriptorSetLayoutBinding meshletDataBinding{};

        // If mesh shaders are used the bindings needs to be accessed by mesh shaders, otherwise they will be accessed by vertex shader stage
        if(m_stats.meshShaderSupport)
        {
            CreateDescriptorSetLayoutBinding(shaderDataLayoutBinding, m_varBuffers[0].globalShaderDataBuffer.descriptorBinding, 
            1, m_varBuffers[0].globalShaderDataBuffer.descriptorType, 
            VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

            CreateDescriptorSetLayoutBinding(vertexBufferBinding, m_currentStaticBuffers.vertexBuffer.descriptorBinding, 
            1, m_currentStaticBuffers.vertexBuffer.descriptorType, 
            VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_TASK_BIT_EXT);

            CreateDescriptorSetLayoutBinding(indirectTaskBufferBinding, m_currentStaticBuffers.indirectTaskBuffer.descriptorBinding, 
            1, m_currentStaticBuffers.indirectTaskBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT);
            
            CreateDescriptorSetLayoutBinding(surfaceBufferBinding, m_currentStaticBuffers.surfaceBuffer.descriptorBinding, 
            1, m_currentStaticBuffers.surfaceBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT);

            CreateDescriptorSetLayoutBinding(meshletBufferBinding, m_currentStaticBuffers.meshletBuffer.descriptorBinding, 
            1, m_currentStaticBuffers.meshletBuffer.descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT);

            CreateDescriptorSetLayoutBinding(meshletDataBinding, m_currentStaticBuffers.meshletDataBuffer.descriptorBinding, 
            1, m_currentStaticBuffers.meshletDataBuffer.descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT);
        }
        else
        {
            CreateDescriptorSetLayoutBinding(shaderDataLayoutBinding, m_varBuffers[0].globalShaderDataBuffer.descriptorBinding,
            1, m_varBuffers[0].globalShaderDataBuffer.descriptorType, 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

            CreateDescriptorSetLayoutBinding(vertexBufferBinding, m_currentStaticBuffers.vertexBuffer.descriptorBinding, 1, 
            m_currentStaticBuffers.vertexBuffer.descriptorType, VK_SHADER_STAGE_VERTEX_BIT);

            // This is never used if mesh shading is not supported, this is simply a placeholder
            CreateDescriptorSetLayoutBinding(indirectTaskBufferBinding, m_currentStaticBuffers.indirectTaskBuffer.descriptorBinding, 
            1, m_currentStaticBuffers.indirectTaskBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);

            CreateDescriptorSetLayoutBinding(surfaceBufferBinding, m_currentStaticBuffers.surfaceBuffer.descriptorBinding, 
            1, m_currentStaticBuffers.surfaceBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);

            CreateDescriptorSetLayoutBinding(meshletBufferBinding, m_currentStaticBuffers.meshletBuffer.descriptorBinding, 
            1, m_currentStaticBuffers.meshletBuffer.descriptorType, 
            VK_SHADER_STAGE_COMPUTE_BIT);

            CreateDescriptorSetLayoutBinding(meshletDataBinding, m_currentStaticBuffers.meshletDataBuffer.descriptorBinding, 
            1, m_currentStaticBuffers.meshletDataBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);
        }

        // Sets the binding for the culling data
        VkDescriptorSetLayoutBinding cullingDataLayoutBinding{};
        CreateDescriptorSetLayoutBinding(cullingDataLayoutBinding, m_varBuffers[0].cullingDataBuffer.descriptorBinding, 
        1, m_varBuffers[0].cullingDataBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);

        // Sets the binding for the depth image
        VkDescriptorSetLayoutBinding depthImageBinding{};
        CreateDescriptorSetLayoutBinding(depthImageBinding, 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
        VK_SHADER_STAGE_COMPUTE_BIT);

        // Sets the binding for the render object buffer
        VkDescriptorSetLayoutBinding renderObjectBufferBinding{};
        CreateDescriptorSetLayoutBinding(renderObjectBufferBinding, m_currentStaticBuffers.renderObjectBuffer.descriptorBinding, 
        1, m_currentStaticBuffers.renderObjectBuffer.descriptorType, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        VkDescriptorSetLayoutBinding transformBufferBinding{};
        CreateDescriptorSetLayoutBinding(transformBufferBinding, m_currentStaticBuffers.transformBuffer.descriptorBinding, 
        1, m_currentStaticBuffers.renderObjectBuffer.descriptorType, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        VkDescriptorSetLayoutBinding materialBufferBinding{};
        CreateDescriptorSetLayoutBinding(materialBufferBinding, m_currentStaticBuffers.materialBuffer.descriptorBinding, 
        1, m_currentStaticBuffers.materialBuffer.descriptorType, VK_SHADER_STAGE_FRAGMENT_BIT);

        VkDescriptorSetLayoutBinding indirectDrawBufferBinding{};
        CreateDescriptorSetLayoutBinding(indirectDrawBufferBinding, m_currentStaticBuffers.indirectDrawBuffer.descriptorBinding,
        1, m_currentStaticBuffers.indirectDrawBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);

        VkDescriptorSetLayoutBinding indirectDrawCountBinding{};
        CreateDescriptorSetLayoutBinding(indirectDrawCountBinding, m_currentStaticBuffers.indirectCountBuffer.descriptorBinding, 
        1, m_currentStaticBuffers.indirectCountBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);

        VkDescriptorSetLayoutBinding visibilityBufferBinding{};
        CreateDescriptorSetLayoutBinding(visibilityBufferBinding, m_currentStaticBuffers.visibilityBuffer.descriptorBinding, 
        1, m_currentStaticBuffers.visibilityBuffer.descriptorType, VK_SHADER_STAGE_COMPUTE_BIT);
        
        // All bindings combined to create the global shader data descriptor set layout
        VkDescriptorSetLayoutBinding shaderDataBindings[14] = {shaderDataLayoutBinding, vertexBufferBinding, 
        cullingDataLayoutBinding, depthImageBinding, renderObjectBufferBinding, transformBufferBinding, materialBufferBinding, 
        indirectDrawBufferBinding, indirectTaskBufferBinding, indirectDrawCountBinding, visibilityBufferBinding, 
        surfaceBufferBinding, meshletBufferBinding, meshletDataBinding};
        m_pushDescriptorBufferLayout = CreateDescriptorSetLayout(m_device, 14, shaderDataBindings, 
        VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if(m_pushDescriptorBufferLayout == VK_NULL_HANDLE)
            return 0;

        // Descriptor set layout for textures
        VkDescriptorSetLayoutBinding texturesLayoutBinding{};
        CreateDescriptorSetLayoutBinding(texturesLayoutBinding, 0, static_cast<uint32_t>(textureCount), 
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        m_textureDescriptorSetlayout = CreateDescriptorSetLayout(m_device, 1, &texturesLayoutBinding);
        if(m_textureDescriptorSetlayout == VK_NULL_HANDLE)
            return 0;

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
        if(m_depthPyramidDescriptorLayout == VK_NULL_HANDLE)
            return 0;

        // The graphics pipeline will use 2 layouts, the one for push desciptors and the constant one for textures
        VkDescriptorSetLayout layouts[2] = { m_pushDescriptorBufferLayout, m_textureDescriptorSetlayout };
        if(!CreatePipelineLayout(m_device, &m_opaqueGeometryPipelineLayout, 2, layouts, 0, nullptr))
            return 0;

        // The layout for culling shaders uses the push descriptor layout but accesses more bindings for culling data and the depth pyramid
        VkPushConstantRange lateCullShaderPostPassPushConstant{};
        CreatePushConstantRange(lateCullShaderPostPassPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t));
        if(!CreatePipelineLayout(m_device, &m_drawCullPipelineLayout, 1, &m_pushDescriptorBufferLayout, 1, &lateCullShaderPostPassPushConstant))
            return 0;

        // The depth pyramid shader uses the set with depth pyramid images and depth attachment image, 
        // It also needs a push constant for the width and height of the current mip level of the depth pyramid
        VkPushConstantRange depthPyramidMipExtentPushConstant{};
        CreatePushConstantRange(depthPyramidMipExtentPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(BlitML::vec2));
        if(!CreatePipelineLayout(m_device, &m_depthPyramidGenerationPipelineLayout, 1, &m_depthPyramidDescriptorLayout, 
        1, &depthPyramidMipExtentPushConstant))
            return 0;

        return 1;
    }

    void VulkanRenderer::UploadTexture(BlitzenEngine::TextureStats& newTexture, VkFormat format)
    {
        CreateTextureImage(reinterpret_cast<void*>(newTexture.pTextureData), m_device, m_allocator, 
        loadedTextures[textureCount].image, 
        {(uint32_t)newTexture.textureWidth, (uint32_t)newTexture.textureHeight, 1}, format, 
        VK_IMAGE_USAGE_SAMPLED_BIT, m_frameToolsList[0].commandBuffer, m_graphicsQueue.handle, 1);
        
        loadedTextures[textureCount].sampler = m_placeholderSampler;

        ++textureCount;
    }

    uint8_t VulkanRenderer::UploadDDSTexture(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10, 
    void* pData, const char* filepath)
    {
        // Create a big buffer to hold the texture data temporarily. It will pass it later
        // This buffer has a random big size, this is because I need to create it before I can get the size needed for the texture image
        BlitzenVulkan::AllocatedBuffer stagingBuffer;
        if(!CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VMA_MEMORY_USAGE_CPU_TO_GPU, 128 * 1024 * 1024, VMA_ALLOCATION_CREATE_MAPPED_BIT))
        {
            BLIT_ERROR("Failed to create staging buffer for texture data copy")
            return 0;
        }
        pData = stagingBuffer.allocationInfo.pMappedData;

        // Calls the function to initialize header, header10 for DDS, get the data of the image and the image format
        unsigned int format = VK_FORMAT_UNDEFINED;
        if(!BlitzenEngine::LoadDDSImage(filepath, header, header10, format, BlitzenEngine::RendererToLoadDDS::Vulkan, pData))
        {
            BLIT_ERROR("Failed to load texture image")
            return 0;
        }
        
        // Casts the placeholder format to a VkFormat
        VkFormat vkFormat = static_cast<VkFormat>(format);

        // Creates the texture image for Vulkan. This function also copies the data of the staging buffer to the image
        if(!CreateTextureImage(stagingBuffer, m_device, m_allocator, loadedTextures[textureCount].image, 
        {header.dwWidth, header.dwHeight, 1}, vkFormat, VK_IMAGE_USAGE_SAMPLED_BIT, 
        m_frameToolsList[0].commandBuffer, m_graphicsQueue.handle, header.dwMipMapCount))
        {
            BLIT_ERROR("Failed to load Vulkan texture image")
            return 0;
        }
        
        // Add the global sampler at the element in the array that was just porcessed
        loadedTextures[textureCount].sampler = m_placeholderSampler;

        textureCount++;

        return 1;
    }


    uint8_t VulkanRenderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources)
    {
        // Creates all know descriptor layouts for all known pipelines
        if(!CreateDescriptorLayouts())
        {
            BLIT_ERROR("Failed to create descriptor set layouts")
            return 0;
        }

        // Creates the uniform buffers
        if(!VarBuffersInit())
        {
            BLIT_ERROR("Failed to create uniform buffers")
            return 0;
        }

        // Upload static data to gpu (though some of these might not be static in the future)
        if(!UploadDataToGPU(pResources->vertices, pResources->indices, pResources->renders, pResources->renderObjectCount,
        pResources->materials, pResources->materialCount, pResources->meshlets, pResources->meshletData, 
        pResources->surfaces, pResources->transforms))
        {
            BLIT_ERROR("Failed to upload data to the GPU")
            return 0;
        }
        
        #ifdef NDEBUG
        // Creates pipeline for The initial culling shader that will be dispatched before the 1st pass. 
        // It performs frustum culling on objects that were visible last frame (visibility is set by the late culling shader)
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/InitialDrawCull.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
        m_drawCullPipelineLayout, &m_initialDrawCullPipeline))
        {
            BLIT_ERROR("Failed to create InitialDrawCull.comp shader program")
            return 0;
        }
        #else
        // Creates pipeline for The initial culling shader that will be dispatched before the 1st pass. 
        // It performs frustum culling on objects that were visible last frame (visibility is set by the late culling shader)
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/InitialDrawCullDebug.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
        m_drawCullPipelineLayout, &m_initialDrawCullPipeline))
        {
            BLIT_ERROR("Failed to create InitialDrawCull.comp shader program")
            return 0;
        }
        #endif
        
        // Creates pipeline for the depth pyramid generation shader which will be dispatched before the late culling compute shader
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/DepthPyramidGeneration.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
        m_depthPyramidGenerationPipelineLayout, &m_depthPyramidGenerationPipeline))
        {
            BLIT_ERROR("Failed to create DepthPyramidGeneration.comp shader program")
            return 0;
        }
        
        #ifdef NDEBUG
        // Creates pipeline for the late culling shader that will be dispatched before the 2nd render pass.
        // It performs frustum culling and occlusion culling on all objects.
        // It creates a draw command for the objects that were not tested by the previous shader
        // It also sets the visibility of each object for this frame, so that it can be accessed next frame
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/LateDrawCull.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
        m_drawCullPipelineLayout, &m_lateDrawCullPipeline))
        {
            BLIT_ERROR("Failed to create LateDrawCull.comp shader program")
            return 0;
        }
        #else
        if(!CreateComputeShaderProgram(m_device, "VulkanShaders/LateDrawCullDebug.comp.glsl.spv", VK_SHADER_STAGE_COMPUTE_BIT, "main", 
        m_drawCullPipelineLayout, &m_lateDrawCullPipeline))
        {
            BLIT_ERROR("Failed to create LateDrawCull.comp shader program")
            return 0;
        }
        #endif
        
        // Create the graphics pipeline object 
        if(!SetupMainGraphicsPipeline())
        {
            BLIT_ERROR("Failed to create the primary graphics pipeline object")
            return 0;
        }

        // culing data values that need to be handled by the renderer itself
        pResources->cullingData.pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
        pResources->cullingData.pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);

        // Since most of these descriptor writes are static they can be initialized here
        pushDescriptorWritesGraphics[0] = {};// This will be where the global shader data write will be, but this one is not always static
        pushDescriptorWritesGraphics[1] = m_currentStaticBuffers.vertexBuffer.descriptorWrite; 
        pushDescriptorWritesGraphics[2] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[3] = m_currentStaticBuffers.transformBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[4] = m_currentStaticBuffers.materialBuffer.descriptorWrite; 
        pushDescriptorWritesGraphics[5] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;

        pushDescriptorWritesCompute[0] = {};// This will be where the global shader data write will be, but this one is not always static
        pushDescriptorWritesCompute[1] = {};
        pushDescriptorWritesCompute[2] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[3] = m_currentStaticBuffers.transformBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[4] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
        pushDescriptorWritesCompute[5] = m_currentStaticBuffers.indirectCountBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[6] = m_currentStaticBuffers.visibilityBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[7] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[8] = {};

        return 1;
    }

    uint8_t VulkanRenderer::UploadDataToGPU(BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices, 
    BlitzenEngine::RenderObject* pRenderObjects, size_t renderObjectCount, BlitzenEngine::Material* pMaterials, size_t materialCount, 
    BlitCL::DynamicArray<BlitzenEngine::Meshlet>& meshlets, BlitCL::DynamicArray<uint32_t>& meshletData, 
    BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface>& surfaces, BlitCL::DynamicArray<BlitzenEngine::MeshTransform>& transforms)
    {
        // Creates a storage buffer that will hold the vertices
        VkDeviceSize vertexBufferSize = sizeof(BlitzenEngine::Vertex) * vertices.GetSize();
        // Fails if there are no vertices
        if(vertexBufferSize == 0)
            return 0;
        // Creates a staging buffer to hold the vertex data and pass it to the vertex buffer later
        AllocatedBuffer stagingVertexBuffer;
        // Initializes the push descritpor buffer struct that holds the vertex buffer
        if(!SetupPushDescriptorBuffer(m_device, m_allocator, m_currentStaticBuffers.vertexBuffer, stagingVertexBuffer, 
        vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vertices.Data()))
            return 0;

        // Creates an index buffer that will hold all the loaded indices
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.GetSize();
        // Fails if there are no indices
        if(indexBufferSize == 0)
            return 0;
        // Creates a staging buffer to hold the index data and pass it to the index buffer later
        AllocatedBuffer stagingIndexBuffer;
        CreateStorageBufferWithStagingBuffer(m_allocator, m_device, indices.Data(), m_currentStaticBuffers.indexBuffer, 
        stagingIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, indexBufferSize);
        // Checks if the above function failed
        if(m_currentStaticBuffers.indexBuffer.buffer == VK_NULL_HANDLE)
            return 0;

        // Creates an SSBO that will hold all the render objects that were loaded for the scene
        VkDeviceSize renderObjectBufferSize = sizeof(BlitzenEngine::RenderObject) * renderObjectCount;
        if(renderObjectBufferSize == 0)
            return 0;
        // Creates a staging buffer to hold the render object data and pass it to the render object buffer later
        AllocatedBuffer renderObjectStagingBuffer;
        if(!SetupPushDescriptorBuffer(m_device, m_allocator, m_currentStaticBuffers.renderObjectBuffer, renderObjectStagingBuffer, 
        renderObjectBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, pRenderObjects))
            return 0;

        // Creates an SSBO that will hold all the mesh surfaces / primitives that were loaded to the scene
        VkDeviceSize surfaceBufferSize = sizeof(BlitzenEngine::PrimitiveSurface) * surfaces.GetSize();
        if(surfaceBufferSize == 0)
            return 0;
        // Creates a staging buffer that will hold the surface data and pass it to the surface buffer later
        AllocatedBuffer surfaceStagingBuffer;
        // Initializes the push descriptor buffer that holds the surface buffer
        if(!SetupPushDescriptorBuffer(m_device, m_allocator, m_currentStaticBuffers.surfaceBuffer, surfaceStagingBuffer, 
        surfaceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, surfaces.Data()))
            return 0;

        // Creates an SSBO that will hold all the materials that were loaded for the scene
        VkDeviceSize materialBufferSize = sizeof(BlitzenEngine::Material) * materialCount;
        if(materialBufferSize == 0)
            return 0;
        // Creates a staging buffer that will hold the material data and pass it to the material buffer later
        AllocatedBuffer materialStagingBuffer; 
        // Initializes the push descriptor buffer that holds the material buffer
        if(!SetupPushDescriptorBuffer(m_device, m_allocator, m_currentStaticBuffers.materialBuffer, materialStagingBuffer, 
        materialBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, pMaterials))
            return 0;

        // Create an SSBO that will hold all the object transforms that were loaded for the scene
        VkDeviceSize transformBufferSize = sizeof(BlitzenEngine::MeshTransform) * transforms.GetSize();
        if(transformBufferSize == 0)
            return 0;
        // Creates a staging buffer that will hold the transform data and pass it to the transform buffer later
        AllocatedBuffer transformStagingBuffer; 
        if(!SetupPushDescriptorBuffer(m_device, m_allocator, m_currentStaticBuffers.transformBuffer, transformStagingBuffer, 
        transformBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, transforms.Data()))
            return 0;

        // Creates an SSBO that will hold all clusters / meshlets that were loaded for the scene
        VkDeviceSize meshletBufferSize = sizeof(BlitzenEngine::Meshlet) * meshlets.GetSize();
        if(meshletBufferSize == 0)
            return 0;
        // Creates a staging buffer that will hold the cluster data and pass it to the cluster buffer later
        AllocatedBuffer meshletStagingBuffer;
        // Initializes the push descriptor buffer that holds the meshlet buffer
        if(!SetupPushDescriptorBuffer(m_device, m_allocator, m_currentStaticBuffers.meshletBuffer, meshletStagingBuffer, 
        meshletBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, meshlets.Data()))
            return 0;

        // Creates an SSBO that will hold all the meshlet indices to the index buffer
        VkDeviceSize meshletDataBufferSize = sizeof(uint32_t) * meshletData.GetSize();
        if(meshletDataBufferSize == 0)
            return 0;
        // Creates a staging buffer that will hold the meshlet indices and pass it to meshlet data buffer later
        AllocatedBuffer meshletDataStagingBuffer;
        // Initializes the push descriptor buffer that holds the meshlet data buffer
        if(!SetupPushDescriptorBuffer(m_device, m_allocator, m_currentStaticBuffers.meshletDataBuffer, meshletDataStagingBuffer, 
        meshletDataBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, meshletData.Data()))
            return 0;

        // Creates the buffer that will hold the indirect draw commands. It is set as an SSBO as well so that it can be written by the culling shaders
        VkDeviceSize indirectDrawBufferSize = sizeof(IndirectDrawData) * renderObjectCount;
        if(indirectDrawBufferSize == 0)
            return 0;
        // Initializes the push descriptor buffer that holds the indirect draw buffer
        if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_currentStaticBuffers.indirectDrawBuffer, 
        indirectDrawBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
            return 0;

        // This is the same thing as the above but for mesh shaders, which do not work for the time being
        VkDeviceSize indirectTaskBufferSize = sizeof(IndirectTaskData) * renderObjectCount;
        if(indirectTaskBufferSize == 0)
            return 0;
        // Initializes the push descriptor buffer that holds the indirect task buffer
        if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_currentStaticBuffers.indirectTaskBuffer, 
        indirectTaskBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
            return 0;

        // Initializes the push descriptor buffer that holds the indirect count buffer
        if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_currentStaticBuffers.indirectCountBuffer, 
        sizeof(uint32_t), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
            return 0;

        // Creates an SSBO that will hold one integer for each object indicating if they were visible or not on the previous frame
        VkDeviceSize visibilityBufferSize = sizeof(uint32_t) * renderObjectCount;
        if(visibilityBufferSize == 0)
            return 0;
        if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_currentStaticBuffers.visibilityBuffer, 
        visibilityBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
                return 0;

        VkCommandBuffer& commandBuffer = m_frameToolsList[0].commandBuffer;

        // Start recording the transfer commands
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        // Copies the data held by the staging buffer to the vertex buffer
        CopyBufferToBuffer(commandBuffer, stagingVertexBuffer.buffer, 
        m_currentStaticBuffers.vertexBuffer.buffer.buffer, vertexBufferSize, 
        0, 0);

        // Copies the index data held by the staging buffer to the index buffer
        CopyBufferToBuffer(commandBuffer, stagingIndexBuffer.buffer, 
        m_currentStaticBuffers.indexBuffer.buffer, indexBufferSize, 
        0, 0);

        // Copies the render object data held by the staging buffer to the render object buffer
        CopyBufferToBuffer(commandBuffer, renderObjectStagingBuffer.buffer, 
        m_currentStaticBuffers.renderObjectBuffer.buffer.buffer, renderObjectBufferSize, 
        0, 0);

        // Copies the surface data held by the staging buffer to the surface buffer
        CopyBufferToBuffer(commandBuffer, surfaceStagingBuffer.buffer, 
        m_currentStaticBuffers.surfaceBuffer.buffer.buffer, surfaceBufferSize, 
        0, 0);

        // Copies the material data held by the staging buffer to the material buffer
        CopyBufferToBuffer(commandBuffer, materialStagingBuffer.buffer, 
        m_currentStaticBuffers.materialBuffer.buffer.buffer, materialBufferSize, 
        0, 0);

        // Copies the transform data held by the staging buffer to the transform buffer
        CopyBufferToBuffer(commandBuffer, transformStagingBuffer.buffer,
        m_currentStaticBuffers.transformBuffer.buffer.buffer, transformBufferSize, 
        0, 0);

        // Copies the cluster data held by the staging buffer to the meshlet buffer
        CopyBufferToBuffer(commandBuffer, meshletStagingBuffer.buffer, 
        m_currentStaticBuffers.meshletBuffer.buffer.buffer, meshletBufferSize, 
        0, 0);

        CopyBufferToBuffer(commandBuffer, meshletDataStagingBuffer.buffer, 
        m_currentStaticBuffers.meshletDataBuffer.buffer.buffer, meshletDataBufferSize, 
        0, 0);

        // The visibility buffer will start the 1st frame with only zeroes(nothing will be drawn on the first frame but that is fine)
        vkCmdFillBuffer(commandBuffer, m_currentStaticBuffers.visibilityBuffer.buffer.buffer, 0, visibilityBufferSize, 0);
        
        // Submit the commands and wait for the queue to finish
        SubmitCommandBuffer(m_graphicsQueue.handle, commandBuffer);
        vkQueueWaitIdle(m_graphicsQueue.handle);

        // Fails if there are no textures to load
        if(textureCount == 0)
            return 0;

        // The descriptor will have multiple descriptors of combined image sampler type. The count is derived from the amount of textures loaded
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = static_cast<uint32_t>(textureCount);

        // Creates the descriptor pool for the textures
        m_textureDescriptorPool = CreateDescriptorPool(m_device, 1, &poolSize, 
        1);
        if(m_textureDescriptorPool == VK_NULL_HANDLE)
            return 0;
 
        // Allocates the descriptor set that will be used to bind the textures
        if(!AllocateDescriptorSets(m_device, m_textureDescriptorPool, &m_textureDescriptorSetlayout, 
        1, &m_textureDescriptorSet))
            return 0;

        // Create image infos for every texture to be passed to the VkWriteDescriptorSet
        BlitCL::DynamicArray<VkDescriptorImageInfo> imageInfos(textureCount);
        for(size_t i = 0; i < imageInfos.GetSize(); ++i)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = loadedTextures[i].image.imageView;
            imageInfos[i].sampler = loadedTextures[i].sampler;
        }

        // Update every descriptor set so that it is available at draw time
        VkWriteDescriptorSet write{};
        WriteImageDescriptorSets(write, imageInfos.Data(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_textureDescriptorSet, 
        static_cast<uint32_t>(imageInfos.GetSize()), 0);
        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

        return 1;
    }



    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        Every operation needed for drawing a single frame is put here
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    void VulkanRenderer::DrawFrame(BlitzenEngine::RenderContext& context)
    {
        if(context.windowResize)
        {
            RecreateSwapchain(context.windowWidth, context.windowHeight);
        }

        // Gets a ref to the frame tools of the current frame
        FrameTools& fTools = m_frameToolsList[m_currentFrame];
        VarBuffers& vBuffers = m_varBuffers[m_currentFrame];

        BlitzenEngine::CullingData& cullData = context.cullingData;
        BlitzenEngine::GlobalShaderData& shaderData = context.globalShaderData;

        // Handle the pyramid width if the window resized
        if(context.windowResize)
        {
            cullData.pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
            cullData.pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);
        }

        // Transparent objects will only be renderered on the 2nd pass
        cullData.postPass = 0;

        // Specifies the descriptor writes that are not static again
        pushDescriptorWritesGraphics[0] = vBuffers.globalShaderDataBuffer.descriptorWrite;
        pushDescriptorWritesCompute[0] = vBuffers.globalShaderDataBuffer.descriptorWrite;
        pushDescriptorWritesCompute[1] = vBuffers.cullingDataBuffer.descriptorWrite;
        
        // Waits for the fence in the current frame tools struct to be signaled and resets it for next time when it gets signalled
        vkWaitForFences(m_device, 1, &(fTools.inFlightFence), VK_TRUE, 1000000000);
        VK_CHECK(vkResetFences(m_device, 1, &(fTools.inFlightFence)))

        // Write the data to the buffer pointers
        *(vBuffers.globalShaderDataBuffer.pData) = shaderData;
        *(vBuffers.cullingDataBuffer.pData) = cullData;
        

        // Asks for the next image in the swapchain to use for presentation, and saves it in swapchainIdx
        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_initHandles.swapchain, 1000000000, fTools.imageAcquiredSemaphore, VK_NULL_HANDLE, &swapchainIdx);


        // The commands start being recorder here (stop when submit is called)
        BeginCommandBuffer(fTools.commandBuffer, 0);

        // Dispatch the compute shader to do early culling 
        //(only perform  frustum culling and LOD selection on objects that were visible on the previous frame)
        DispatchRenderObjectCullingComputeShader(fTools.commandBuffer, m_initialDrawCullPipeline, 
        (cullData.drawCount / 64) + 1, pushDescriptorWritesCompute);



        // The viewport and scissor are dynamic, so they should be set here
        DefineViewportAndScissor(fTools.commandBuffer, m_drawExtent);

        // Before beginning the first render pass, the rendering attachment need to transition to a suitable layout
        // First the color attachment
        VkImageMemoryBarrier2 colorAttachmentTransitionBarrier{};
        ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentTransitionBarrier, 
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 
        VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, 
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

        // Second the depth attachment
        VkImageMemoryBarrier2 depthAttachmentTransitionBarrier{};
        ImageMemoryBarrier(m_depthAttachment.image, depthAttachmentTransitionBarrier, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);

        // Places the 2 image barriers
        VkImageMemoryBarrier2 memoryBarriers[2] = { colorAttachmentTransitionBarrier, depthAttachmentTransitionBarrier };
        PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, memoryBarriers);

        // Draws the object that passed the first culling compute shader
        // (Specifically the objects that passed frustum culling and were visible last frame)
        DrawGeometry(fTools.commandBuffer, pushDescriptorWritesGraphics, cullData.drawCount, 0, m_opaqueGeometryPipeline);

        // The first render pass ends here
        vkCmdEndRendering(fTools.commandBuffer);


        
        /*
            Before the late render pass can begin a depth pyramid needs to be created based on the previous render pass depth buffer
        */
        // Image memory barrier to wait for the previous render pass to be done with the depth attachment and transition it shader read layout
        VkImageMemoryBarrier2 shaderDepthBarrier{};
        ImageMemoryBarrier(m_depthAttachment.image, shaderDepthBarrier, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, 
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, 
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 
        0, VK_REMAINING_MIP_LEVELS);

        // Image memory barrier to wait for the previous frame to be done with the depth pyramid and transition it to general layout
        VkImageMemoryBarrier2 depthPyramidFirstBarrier{};
        ImageMemoryBarrier(m_depthPyramid.image, depthPyramidFirstBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

        // Create a pipeline barrier for the above
        VkImageMemoryBarrier2 depthPyramidSetBarriers[2] = {shaderDepthBarrier, depthPyramidFirstBarrier};
        PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, depthPyramidSetBarriers);

        // Bind the compute pipeline, the depth pyramid will be called for as many depth pyramid mip levels there are
        vkCmdBindPipeline(fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_depthPyramidGenerationPipeline);

        // Call the depth pyramid creation compute shader for every mip level in the depth pyramid
        for(size_t i = 0; i < m_depthPyramidMipLevels; ++i)
        {
            // Pass the depth attachment image view or the previous image view of the depth pyramid as the image to be read by the shader
            VkDescriptorImageInfo sourceImageInfo{};
            VkWriteDescriptorSet write{};
            WriteImageDescriptorSets(write, sourceImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE, 1, 
            (i == 0) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL, 
            (i == 0) ? m_depthAttachment.imageView : m_depthPyramidMips[i - 1], m_depthAttachmentSampler);

            // Pass the current depth pyramid image view to the shader as the output
            VkDescriptorImageInfo outImageInfo{};
            VkWriteDescriptorSet write2{};
            WriteImageDescriptorSets(write2, outImageInfo, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, 0, 
            VK_IMAGE_LAYOUT_GENERAL, m_depthPyramidMips[i]);
            VkWriteDescriptorSet writes[2] = {write, write2};

            // Push the descriptor sets
            PushDescriptors(m_initHandles.instance, fTools.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_depthPyramidGenerationPipelineLayout, 
            0, 2, writes);

            // Calculate the extent of the current depth pyramid mip level
            uint32_t levelWidth = BlitML::Max(1u, (m_depthPyramidExtent.width) >> i);
            uint32_t levelHeight = BlitML::Max(1u, (m_depthPyramidExtent.height) >> i);
            // Pass the extent to the push constant
            BlitML::vec2 pyramidLevelExtentPushConstant(static_cast<float>(levelWidth), static_cast<float>(levelHeight));
            vkCmdPushConstants(fTools.commandBuffer, m_depthPyramidGenerationPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 
            sizeof(BlitML::vec2), &pyramidLevelExtentPushConstant);

            // Dispatch the shader
            vkCmdDispatch(fTools.commandBuffer, levelWidth / 32 + 1, levelHeight / 32 + 1, 1);

            // Wait for the shader to finish before the next loop calls it again (or before the culing shader accesses it if the loop ends)
            VkImageMemoryBarrier2 dispatchWriteBarrier{};
            ImageMemoryBarrier(m_depthPyramid.image, dispatchWriteBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, 
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
            PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &dispatchWriteBarrier);
        }

        VkWriteDescriptorSet depthPyramidWrite{};
        VkDescriptorImageInfo depthPyramidImageInfo{};
        // Add the depth image to the push descriptors for the culling shader
        WriteImageDescriptorSets(depthPyramidWrite, depthPyramidImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_NULL_HANDLE, 
        3, VK_IMAGE_LAYOUT_GENERAL, m_depthPyramid.imageView, m_depthAttachmentSampler);
        pushDescriptorWritesCompute[8] = depthPyramidWrite;

        // Dispatches the compute shader to do culling on objects that were not visible last frame. 
        // It will do both occusion and frustum culling and LOD selection
        // It also performs these operations on objects that were visible last frame, but it only updates their visibility buffer
        DispatchRenderObjectCullingComputeShader(fTools.commandBuffer, m_lateDrawCullPipeline, 
        cullData.drawCount / 64 + 1, pushDescriptorWritesCompute, 1, 0);

        // Wait for the culling compute shader to be done with the depth attachment and transition it to depth attahcment layout
        VkImageMemoryBarrier2 depthAttachmentReadBarrier{};
        ImageMemoryBarrier(m_depthAttachment.image, depthAttachmentReadBarrier, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
        | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &depthAttachmentReadBarrier);

        // Draws the objects that passed the late culling compute shader
        DrawGeometry(fTools.commandBuffer, pushDescriptorWritesGraphics, cullData.drawCount, 1, m_opaqueGeometryPipeline);

        // End of late render pass
        vkCmdEndRendering(fTools.commandBuffer);



        // Dispatch the late culling shader again, tihis time to cull transparent objects
        //vBuffers.cullingDataBuffer.pData->postPass = 1;
        DispatchRenderObjectCullingComputeShader(fTools.commandBuffer, m_lateDrawCullPipeline, 
        cullData.drawCount / 64 + 1, pushDescriptorWritesCompute, 1, 1);

        DrawGeometry(fTools.commandBuffer, pushDescriptorWritesGraphics, cullData.drawCount, 1, m_postPassGeometryPipeline);
        
        // End of post pass
        vkCmdEndRendering(fTools.commandBuffer);


        /*
            The final set of commands is for the color attachment to be copied to the current swapchain image
        */
        // Create an image barrier for the color attachment to transition its layout to transfer source optimal
        VkImageMemoryBarrier2 colorAttachmentTransferBarrier{};
        ImageMemoryBarrier(m_colorAttachment.image, colorAttachmentTransferBarrier, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, 
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

        // Create an image barrier for the swapchain image to transition its layout to transfer dst optimal
        VkImageMemoryBarrier2 swapchainImageTransferBarrier{};
        ImageMemoryBarrier(m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], swapchainImageTransferBarrier, 
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, 
        VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);

        // Set the pipeline barrier with the 2 image barriers
        VkImageMemoryBarrier2 firstTransferMemoryBarriers[2] = {colorAttachmentTransferBarrier, swapchainImageTransferBarrier};
        PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 2, firstTransferMemoryBarriers);

        // Old debug code that display the debug pyramid instead of the image, does not work anymore for reason I'm not sure of
        if(0)// This should say if(context.debugPyramid)
        {
            uint32_t debugLevel = 0;
            CopyImageToImage(fTools.commandBuffer, m_depthPyramid.image, VK_IMAGE_LAYOUT_GENERAL, 
            m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            {uint32_t(BlitML::Max(1u, (m_depthPyramidExtent.width) >> debugLevel)), uint32_t(BlitML::Max(1u, (m_depthPyramidExtent.height) >> debugLevel))}, 
            m_initHandles.swapchainExtent, {VK_IMAGE_ASPECT_COLOR_BIT, debugLevel, 0, 1}, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, VK_FILTER_NEAREST);
        }
        // Copy the color attachment to the swapchain image
        else
        {
            CopyImageToImage(fTools.commandBuffer, m_colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
            m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_drawExtent, 
            m_initHandles.swapchainExtent, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, VK_FILTER_LINEAR);
        }

        // Create a barrier for the swapchain image to transition to present optimal
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(m_initHandles.swapchainImages[static_cast<size_t>(swapchainIdx)], presentImageBarrier, VK_PIPELINE_STAGE_2_BLIT_BIT, 
        VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(fTools.commandBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);

        // All commands have ben recorded, the command buffer is submitted
        SubmitCommandBuffer(m_graphicsQueue.handle, fTools.commandBuffer, 1, fTools.imageAcquiredSemaphore, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, 
        1, fTools.readyToPresentSemaphore, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, fTools.inFlightFence);

        // Presents the swapchain image, so that the rendering results are shown on the window
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &fTools.readyToPresentSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &(m_initHandles.swapchain);
        presentInfo.pImageIndices = &swapchainIdx;
        vkQueuePresentKHR(m_presentQueue.handle, &presentInfo);

        // Change the current frame to the next frame, important when using double buffering
        m_currentFrame = (m_currentFrame + 1) % BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanRenderer::DispatchRenderObjectCullingComputeShader(VkCommandBuffer commandBuffer, VkPipeline pipeline,
    uint32_t groupCountX, VkWriteDescriptorSet* pWrites, uint8_t lateCulling /*=0*/, uint32_t postPass /*=0*/)
    {
        PushDescriptors(m_initHandles.instance, commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_drawCullPipelineLayout, 
        0, lateCulling ? 9 : 8, pWrites);

        // Wait for previous frame later pass to be done with the indirect count buffer before zeroing it
        VkBufferMemoryBarrier2 waitBeforeZeroingCountBuffer{};
        BufferMemoryBarrier(m_currentStaticBuffers.indirectCountBuffer.buffer.buffer, waitBeforeZeroingCountBuffer,
        VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT, 0, VK_WHOLE_SIZE);
        PipelineBarrier(commandBuffer, 0, nullptr, 1, &waitBeforeZeroingCountBuffer, 0, nullptr);

        // Initialize the indirect count buffer as zero
        vkCmdFillBuffer(commandBuffer, m_currentStaticBuffers.indirectCountBuffer.buffer.buffer, 0, sizeof(uint32_t), 0);

        // Before dispatching the compute shader, create a pipeline barrier so that the buffer is filled with 0, before it's processed
        VkBufferMemoryBarrier2 fillBufferBarrier{};
        BufferMemoryBarrier(m_currentStaticBuffers.indirectCountBuffer.buffer.buffer, fillBufferBarrier, VK_PIPELINE_STAGE_2_TRANSFER_BIT, 
        VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, 
        0, VK_WHOLE_SIZE);
        PipelineBarrier(commandBuffer, 0, nullptr, 1, &fillBufferBarrier, 0, nullptr);

        // Binds the shader's pipeline and dispatches the shader to cull the render objects
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        if(lateCulling)
            vkCmdPushConstants(commandBuffer, m_drawCullPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &postPass);
        vkCmdDispatch(commandBuffer, groupCountX, 1, 1);

        // Stops the indirect stage from reading commands until the compute shader completes
        VkMemoryBarrier2 memoryBarrier{};
        MemoryBarrier(memoryBarrier, 
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, 
        VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, VK_ACCESS_2_MEMORY_READ_BIT);
        PipelineBarrier(commandBuffer, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
    }

    void VulkanRenderer::DrawGeometry(VkCommandBuffer commandBuffer, VkWriteDescriptorSet* pDescriptorWrites, uint32_t drawCount, 
    uint8_t latePass, VkPipeline pipeline)
    {
        // Creates info for the color attachment 
        VkRenderingAttachmentInfo colorAttachmentInfo{};
        CreateRenderingAttachmentInfo(colorAttachmentInfo, m_colorAttachment.imageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
        latePass ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, {0.1f, 0.2f, 0.3f, 0});
        // Creates info for the depth attachment
        VkRenderingAttachmentInfo depthAttachmentInfo{};
        CreateRenderingAttachmentInfo(depthAttachmentInfo, m_depthAttachment.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 
        latePass ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

        // Render pass begins here, the function expects that proper barriers were setup before
        BeginRendering(commandBuffer, m_drawExtent, {0, 0}, 1, &colorAttachmentInfo, 
        &depthAttachmentInfo, nullptr);

        // Pushes all uniform buffer descriptors but the culling data one to the graphics pipelines
        PushDescriptors(m_initHandles.instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
        m_opaqueGeometryPipelineLayout, 0, 7, pDescriptorWrites);

        // Bind the texture descriptor set. This one was allocated and written to in the UploadDataToGPU function
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_opaqueGeometryPipelineLayout, 1,
        1, &m_textureDescriptorSet, 0, nullptr);

        // Bind the graphics pipeline and the index buffer
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindIndexBuffer(commandBuffer, m_currentStaticBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        // Use draw indirect to draw the objects(mesh shading or vertex shader)
        if(m_stats.meshShaderSupport)
        {
            DrawMeshTasks(m_initHandles.instance, commandBuffer, m_currentStaticBuffers.indirectTaskBuffer.buffer.buffer, 
            offsetof(IndirectTaskData, drawIndirectTasks), m_currentStaticBuffers.indirectCountBuffer.buffer.buffer, 0,
            drawCount, sizeof(IndirectTaskData));
        }
        else
        {
            vkCmdDrawIndexedIndirectCount(commandBuffer, m_currentStaticBuffers.indirectDrawBuffer.buffer.buffer, 
            offsetof(IndirectDrawData, drawIndirect), m_currentStaticBuffers.indirectCountBuffer.buffer.buffer, 0,
            drawCount, sizeof(IndirectDrawData));
        }
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
        //m_drawExtent.width = std::min(windowWidth, m_drawExtent.width);
        //m_drawExtent.height = std::min(windowHeight, m_drawExtent.height);

        // Wait for the GPU to be done with the swapchain and destroy the swapchain and depth pyramid
        vkDeviceWaitIdle(m_device);

        vkDestroySwapchainKHR(m_device, m_initHandles.swapchain, nullptr);
        m_initHandles.swapchain = newSwapchain;

        m_depthPyramid.CleanupResources(m_allocator, m_device);
        for(uint8_t i = 0; i < m_depthPyramidMipLevels; ++i)
        {
            vkDestroyImageView(m_device, m_depthPyramidMips[size_t(i)], m_pCustomAllocator);
        }

        // ReCreate the depth pyramid after the old one has been destroyed
        CreateDepthPyramid(m_depthPyramid, m_depthPyramidExtent, m_depthPyramidMips, m_depthPyramidMipLevels, m_depthAttachmentSampler, 
        m_drawExtent, m_device, m_allocator, 0);
    }

    void VulkanRenderer::ClearFrame()
    {
        vkDeviceWaitIdle(m_device);
        FrameTools& fTools = m_frameToolsList[m_currentFrame];
        VkCommandBuffer commandBuffer = m_frameToolsList[m_currentFrame].commandBuffer;

        vkWaitForFences(m_device, 1, &fTools.inFlightFence, VK_TRUE, 1000000000);
        vkResetFences(m_device, 1, &fTools.inFlightFence);

        uint32_t swapchainIdx;
        vkAcquireNextImageKHR(m_device, m_initHandles.swapchain, 1000000000, fTools.imageAcquiredSemaphore, VK_NULL_HANDLE, &swapchainIdx);

        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkImageMemoryBarrier2 colorAttachmentBarrier{};
        ImageMemoryBarrier(m_initHandles.swapchainImages[swapchainIdx], colorAttachmentBarrier, 
        VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 
        VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &colorAttachmentBarrier);

        VkClearColorValue value;
        value.float32[0] = 0;
        value.float32[1] = 0;
        value.float32[2] = 0;
        value.float32[3] = 1;
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer = 0;
        range.layerCount = VK_REMAINING_ARRAY_LAYERS;
        vkCmdClearColorImage(commandBuffer, m_initHandles.swapchainImages[swapchainIdx], VK_IMAGE_LAYOUT_GENERAL, &value, 
        1, &range);

        VkImageMemoryBarrier2 swapchainPresentBarrier{};
        ImageMemoryBarrier(m_initHandles.swapchainImages[swapchainIdx], swapchainPresentBarrier, 
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_2_NONE, 
        VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 
        0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(commandBuffer, 0, nullptr, 0, nullptr, 1, &swapchainPresentBarrier);

        // All commands have ben recorded, the command buffer is submitted
        SubmitCommandBuffer(m_graphicsQueue.handle, commandBuffer, 1, fTools.imageAcquiredSemaphore, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, 
        1, fTools.readyToPresentSemaphore, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, fTools.inFlightFence);

        // Presents the swapchain image, so that the rendering results are shown on the window
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &fTools.readyToPresentSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &(m_initHandles.swapchain);
        presentInfo.pImageIndices = &swapchainIdx;
        vkQueuePresentKHR(m_presentQueue.handle, &presentInfo);

        // Reset any fences that were not reset
        for(size_t i = 0; i < BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if(i != m_currentFrame)
                vkResetFences(m_device, 1, &m_frameToolsList[i].inFlightFence);
        }
    }

    void VulkanRenderer::SetupForSwitch(uint32_t windowWidth, uint32_t windowHeight)
    {
        RecreateSwapchain(windowWidth, windowHeight);
    }
}