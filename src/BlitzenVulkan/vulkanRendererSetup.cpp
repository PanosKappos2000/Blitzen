#define VMA_IMPLEMENTATION// Implements vma funcions. Header file included in vulkanData.h
#include "vulkanRenderer.h"

namespace BlitzenVulkan
{
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
        // Checks if resource management has been setup
        if(!m_stats.bResourceManagementReady)
        {
            // Calls the function and checks if it succeeded. If it did not, it fails
            SetupResourceManagement();
            if(!m_stats.bResourceManagementReady)
            {
                BLIT_ERROR("Failed to setup resource management for Vulkan")
                return 0;
            }
        }

        // Creates a big buffer to hold the texture data temporarily. It will pass it later
        // This buffer has a random big size, as it needs to be allocated so that pData is not null in the next function
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

    void VulkanRenderer::SetupResourceManagement()
    {
        // Initializes VMA allocator which will be used to allocate all Vulkan resources
        // Initialized here since textures can and will be given to Vulkan before the renderer is set up, and they need to be allocated
        if(!CreateVmaAllocator(m_device, m_instance, m_physicalDevice, m_allocator, 
        VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT))
        {
            BLIT_ERROR("Failed to create the vma allocator")
            return;
        }

        MemoryCrucialHandles* pMemCrucials = BlitzenCore::GetVulkanMemoryCrucials();
        pMemCrucials->allocator = m_allocator;
        pMemCrucials->device = m_device;
        pMemCrucials->instance = m_instance;
    
        // This sampler will be used by all textures for now
        // Initialized here since textures can and will be given to Vulkan before the renderer is set up
        if(!CreateTextureSampler(m_device, m_placeholderSampler))
            return;

        // Creates command buffers and synchronization structures in the frame tools struct
        // Created on first Vulkan Initialization stage, because can and will be uploaded to Vulkan early
        if (!FrameToolsInit())
        {
            BLIT_ERROR("Failed to create frame tools")
            return;
        }

        // If it goes through everything, it updates the boolean
        m_stats.bResourceManagementReady = 1;
    }

    uint8_t VulkanRenderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources, float& pyramidWidth, float& pyramidHeight)
    {
        // Checks if resource management has been setup
        if(!m_stats.bResourceManagementReady)
        {
            // Calls the function and checks if it succeeded. If it did not, it fails
            SetupResourceManagement();
            if(!m_stats.bResourceManagementReady)
            {
                BLIT_ERROR("Failed to setup resource management for Vulkan")
                return 0;
            }
        }

        // Creates Rendering attachment image resource for color attachment
        if(!CreateImage(m_device, m_allocator, m_colorAttachment, {m_drawExtent.width, m_drawExtent.height, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT))
        {
            BLIT_ERROR("Failed to create color attachment image resource")
            return 0;
        }

        // Creates rendering attachment image resource for depth attachment
        if(!CreateImage(m_device, m_allocator, m_depthAttachment, {m_drawExtent.width, m_drawExtent.height, 1}, VK_FORMAT_D32_SFLOAT, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT))
        {
            BLIT_ERROR("Failed to create depth attachment image resource")
            return 0;
        }

        // Create the depth pyramid image and its mips that will be used for occlusion culling
        if(!CreateDepthPyramid(m_depthPyramid, m_depthPyramidExtent, m_depthPyramidMips, m_depthPyramidMipLevels, m_depthAttachmentSampler, 
        m_drawExtent, m_device, m_allocator))
        {
            BLIT_ERROR("Failed to create the depth pyramid")
            return 0;
        }

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
        pyramidWidth = static_cast<float>(m_depthPyramidExtent.width);
        pyramidHeight = static_cast<float>(m_depthPyramidExtent.height);

        // Since most of these descriptor writes are static they can be initialized here
        pushDescriptorWritesGraphics[0] = {};// This will be where the global shader data write will be, but this one is not always static
        pushDescriptorWritesGraphics[1] = m_currentStaticBuffers.vertexBuffer.descriptorWrite; 
        pushDescriptorWritesGraphics[2] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[3] = m_currentStaticBuffers.transformBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[4] = m_currentStaticBuffers.materialBuffer.descriptorWrite; 
        pushDescriptorWritesGraphics[5] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
        pushDescriptorWritesGraphics[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite;

        pushDescriptorWritesCompute[0] = {};// This will be where the global shader data write will be, but this one is not always static
        pushDescriptorWritesCompute[1] = m_currentStaticBuffers.renderObjectBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[2] = m_currentStaticBuffers.transformBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[3] = m_currentStaticBuffers.indirectDrawBuffer.descriptorWrite;
        pushDescriptorWritesCompute[4] = m_currentStaticBuffers.indirectCountBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[5] = m_currentStaticBuffers.visibilityBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[6] = m_currentStaticBuffers.surfaceBuffer.descriptorWrite; 
        pushDescriptorWritesCompute[7] = {};

        return 1;
    }

    uint8_t VulkanRenderer::FrameToolsInit()
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

        // Semaphore will be the same for both semaphores
        VkSemaphoreCreateInfo semaphoresInfo{};
        semaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoresInfo.flags = 0;
        semaphoresInfo.pNext = nullptr;

        // Creates a set of frame tools for each possible frame in flight
        for(size_t i = 0; i < ce_framesInFlight; ++i)
        {
            FrameTools& frameTools = m_frameToolsList[i];

            // Creates the command pool that manages the command buffer's memory
            VkResult commandPoolResult = vkCreateCommandPool(m_device, &commandPoolsInfo, m_pCustomAllocator, &(frameTools.mainCommandPool));
            if(commandPoolResult != VK_SUCCESS)
                return 0;

            // Allocates the command buffer using the above command pool
            commandBuffersInfo.commandPool = frameTools.mainCommandPool;
            VkResult commandBufferResult = vkAllocateCommandBuffers(m_device, &commandBuffersInfo, &(frameTools.commandBuffer));
            if(commandBufferResult != VK_SUCCESS)
                return 0;

            // Creates the fence that stops the CPU from acquiring a new swapchain image before the GPU is done with the previous frame
            VkResult fenceResult = vkCreateFence(m_device, &fenceInfo, m_pCustomAllocator, &(frameTools.inFlightFence));
            if(fenceResult != VK_SUCCESS)
                return 0;

            // Creates the semaphore that stops command buffer submitting before the next swapchain image is acquired
            VkResult imageSemaphoreResult = vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator, &(frameTools.imageAcquiredSemaphore));
            if(imageSemaphoreResult != VK_SUCCESS)
                return 0;

            // Creates the semaphore that stops presentation before the command buffer is submitted
            VkResult presentSemaphoreResult = vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator, &(frameTools.readyToPresentSemaphore));
            if(presentSemaphoreResult != VK_SUCCESS)
                return 0;
        }

        return 1;
    }

    uint8_t CreateDepthPyramid(AllocatedImage& depthPyramidImage, VkExtent2D& depthPyramidExtent, 
    VkImageView* depthPyramidMips, uint8_t& depthPyramidMipLevels, VkSampler& depthAttachmentSampler, 
    VkExtent2D drawExtent, VkDevice device, VmaAllocator allocator, uint8_t createSampler /* =1 */)
    {
        // This will fail if mip levels do not start from 0
        depthPyramidMipLevels = 0;
    
        // Create the sampler only if it is requested. In case where the depth pyramid is recreated on window resize, this is not needed
        if(createSampler)
            depthAttachmentSampler = CreateSampler(device, VK_SAMPLER_REDUCTION_MODE_MIN);
    
        // Get a conservative starting extent for the depth pyramid image, by getting the previous power of 2 of the draw extent
        depthPyramidExtent.width = BlitML::PreviousPow2(drawExtent.width);
        depthPyramidExtent.height = BlitML::PreviousPow2(drawExtent.height);
    
        // Get the amount of mip levels for the depth pyramid by dividing the extent by 2, until both width and height are not over 1
        uint32_t width = depthPyramidExtent.width;
        uint32_t height = depthPyramidExtent.height;
        while(width > 1 || height > 1)
        {
            depthPyramidMipLevels++;
            width /= 2;
            height /= 2;
        }
    
        // Create the depth pyramid image which will be used as storage image and to then transfer its data for occlusion culling
        if(!CreateImage(device, allocator, depthPyramidImage, {depthPyramidExtent.width, depthPyramidExtent.height, 1}, VK_FORMAT_R32_SFLOAT, 
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, depthPyramidMipLevels))
            return 0;
    
        for(uint8_t i = 0; i < depthPyramidMipLevels; ++i)
        {
            if(!CreateImageView(device, depthPyramidMips[size_t(i)], depthPyramidImage.image, VK_FORMAT_R32_SFLOAT, i, 1))
                return 0;
        }
    
        return 1;
    }

    uint8_t VulkanRenderer::CreateDescriptorLayouts()
    {
        // Binding used by both compute and graphics pipelines, to access global data like the view matrix
        VkDescriptorSetLayoutBinding viewDataLayoutBinding{};
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
            CreateDescriptorSetLayoutBinding(viewDataLayoutBinding, m_varBuffers[0].viewDataBuffer.descriptorBinding, 
            1, m_varBuffers[0].viewDataBuffer.descriptorType, 
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
            CreateDescriptorSetLayoutBinding(viewDataLayoutBinding, m_varBuffers[0].viewDataBuffer.descriptorBinding,
            1, m_varBuffers[0].viewDataBuffer.descriptorType, 
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
        VkDescriptorSetLayoutBinding shaderDataBindings[13] = {viewDataLayoutBinding, vertexBufferBinding, 
        depthImageBinding, renderObjectBufferBinding, transformBufferBinding, materialBufferBinding, 
        indirectDrawBufferBinding, indirectTaskBufferBinding, indirectDrawCountBinding, visibilityBufferBinding, 
        surfaceBufferBinding, meshletBufferBinding, meshletDataBinding};
        m_pushDescriptorBufferLayout = CreateDescriptorSetLayout(m_device, 13, shaderDataBindings, 
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
        CreatePushConstantRange(lateCullShaderPostPassPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(DrawCullShaderPushConstant));
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

    uint8_t VulkanRenderer::VarBuffersInit()
    {
        for(size_t i = 0; i < ce_framesInFlight; ++i)
        {
            VarBuffers& buffers = m_varBuffers[i];

            // Tries to create the global shader data uniform buffer
            if(!CreateBuffer(m_allocator, buffers.viewDataBuffer.buffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
            sizeof(BlitzenEngine::CameraViewData), VMA_ALLOCATION_CREATE_MAPPED_BIT))
                return 0;
            // If everything went fine, get the persistent mapped pointer to the buffer
            buffers.viewDataBuffer.pData = reinterpret_cast<BlitzenEngine::CameraViewData*>(
            buffers.viewDataBuffer.buffer.allocation->GetMappedData());
            // Creates the VkWriteDescriptor for this buffer here, as it will reamain constant 
            WriteBufferDescriptorSets(buffers.viewDataBuffer.descriptorWrite, buffers.viewDataBuffer.bufferInfo, 
            buffers.viewDataBuffer.descriptorType, buffers.viewDataBuffer.descriptorBinding, 
            buffers.viewDataBuffer.buffer.bufferHandle);
        }

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
        if(m_currentStaticBuffers.indexBuffer.bufferHandle == VK_NULL_HANDLE)
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

        // Creates the buffer that will hold the indirect draw commands. It is set as an SSBO as well so that it can be written by the culling shaders
        VkDeviceSize indirectDrawBufferSize = sizeof(IndirectDrawData) * renderObjectCount;
        if(indirectDrawBufferSize == 0)
            return 0;
        // Initializes the push descriptor buffer that holds the indirect draw buffer
        if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_currentStaticBuffers.indirectDrawBuffer, 
        indirectDrawBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
            return 0;

        
        // Create the buffers for cluster if they are needed
        VkDeviceSize indirectTaskBufferSize = sizeof(IndirectTaskData) * renderObjectCount;
        VkDeviceSize meshletBufferSize = sizeof(BlitzenEngine::Meshlet) * meshlets.GetSize();
        AllocatedBuffer meshletStagingBuffer;
        VkDeviceSize meshletDataBufferSize = sizeof(uint32_t) * meshletData.GetSize();
        AllocatedBuffer meshletDataStagingBuffer;
        if(m_stats.meshShaderSupport)
        {
            if(indirectTaskBufferSize == 0)
                return 0;
            // Initializes the push descriptor buffer that holds the indirect task buffer
            if(!SetupPushDescriptorBuffer(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY, m_currentStaticBuffers.indirectTaskBuffer, 
            indirectTaskBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
                return 0;

            // Creates an SSBO that will hold all clusters / meshlets that were loaded for the scene
            if(meshletBufferSize == 0)
                return 0;
            // Initializes the push descriptor buffer that holds the meshlet buffer
            if(!SetupPushDescriptorBuffer(m_device, m_allocator, m_currentStaticBuffers.meshletBuffer, meshletStagingBuffer, 
            meshletBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, meshlets.Data()))
                return 0;

            // Creates an SSBO that will hold all the meshlet indices to the index buffer
            if(meshletDataBufferSize == 0)
                return 0;
            // Initializes the push descriptor buffer that holds the meshlet data buffer
            if(!SetupPushDescriptorBuffer(m_device, m_allocator, m_currentStaticBuffers.meshletDataBuffer, meshletDataStagingBuffer, 
            meshletDataBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, meshletData.Data()))
                return 0;
        }

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
        CopyBufferToBuffer(commandBuffer, stagingVertexBuffer.bufferHandle, 
        m_currentStaticBuffers.vertexBuffer.buffer.bufferHandle, vertexBufferSize, 
        0, 0);

        // Copies the index data held by the staging buffer to the index buffer
        CopyBufferToBuffer(commandBuffer, stagingIndexBuffer.bufferHandle, 
        m_currentStaticBuffers.indexBuffer.bufferHandle, indexBufferSize, 
        0, 0);

        // Copies the render object data held by the staging buffer to the render object buffer
        CopyBufferToBuffer(commandBuffer, renderObjectStagingBuffer.bufferHandle, 
        m_currentStaticBuffers.renderObjectBuffer.buffer.bufferHandle, renderObjectBufferSize, 
        0, 0);

        // Copies the surface data held by the staging buffer to the surface buffer
        CopyBufferToBuffer(commandBuffer, surfaceStagingBuffer.bufferHandle, 
        m_currentStaticBuffers.surfaceBuffer.buffer.bufferHandle, surfaceBufferSize, 
        0, 0);

        // Copies the material data held by the staging buffer to the material buffer
        CopyBufferToBuffer(commandBuffer, materialStagingBuffer.bufferHandle, 
        m_currentStaticBuffers.materialBuffer.buffer.bufferHandle, materialBufferSize, 
        0, 0);

        // Copies the transform data held by the staging buffer to the transform buffer
        CopyBufferToBuffer(commandBuffer, transformStagingBuffer.bufferHandle,
        m_currentStaticBuffers.transformBuffer.buffer.bufferHandle, transformBufferSize, 
        0, 0);
        
        if(m_stats.meshShaderSupport)
        {
            // Copies the cluster data held by the staging buffer to the meshlet buffer
            CopyBufferToBuffer(commandBuffer, meshletStagingBuffer.bufferHandle, 
            m_currentStaticBuffers.meshletBuffer.buffer.bufferHandle, meshletBufferSize, 
            0, 0);

            CopyBufferToBuffer(commandBuffer, meshletDataStagingBuffer.bufferHandle, 
            m_currentStaticBuffers.meshletDataBuffer.buffer.bufferHandle, meshletDataBufferSize, 
            0, 0);
        }

        // The visibility buffer will start the 1st frame with only zeroes(nothing will be drawn on the first frame but that is fine)
        vkCmdFillBuffer(commandBuffer, m_currentStaticBuffers.visibilityBuffer.buffer.bufferHandle, 0, visibilityBufferSize, 0);
        
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
}