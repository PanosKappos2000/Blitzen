#include "vulkanRenderer.h"
#include "Platform/filesystem.h"
#include "vulkanPipelines.h"

namespace BlitzenVulkan
{
    void GetDefaultPipelineInfo(VkGraphicsPipelineCreateInfo& pipelineInfo, 
        VkPipelineRenderingCreateInfo& dynamicRenderingInfo, VkFormat* pFormat, 
        VkPipelineInputAssemblyStateCreateInfo& inputAssembly, 
        VkPipelineViewportStateCreateInfo& viewport, 
        VkPipelineDynamicStateCreateInfo& dynamicState, 
        VkPipelineRasterizationStateCreateInfo& rasterization, 
		VkPipelineMultisampleStateCreateInfo& multisampling,
        VkPipelineDepthStencilStateCreateInfo& depthState, 
        VkPipelineColorBlendAttachmentState& colorBlendAttachment,
        VkPipelineColorBlendStateCreateInfo& colorBlendState, 
        VkPipelineVertexInputStateCreateInfo& vertexInput, 
        VkDynamicState* pDynamicStates
    )
    {
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.flags = 0;
        pipelineInfo.renderPass = VK_NULL_HANDLE;
        
        dynamicRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        dynamicRenderingInfo.colorAttachmentCount = 1;
        dynamicRenderingInfo.pColorAttachmentFormats = pFormat;
        dynamicRenderingInfo.depthAttachmentFormat = ce_depthAttachmentFormat;
        pipelineInfo.pNext = &dynamicRenderingInfo;

        // Setting up triangle primitive assembly
        inputAssembly = SetTriangleListInputAssembly();
        pipelineInfo.pInputAssemblyState = &inputAssembly;

        // Dynamic viewport by default
        SetDynamicStateViewport(pDynamicStates, viewport, dynamicState);
        pipelineInfo.pViewportState = &viewport;
        pipelineInfo.pDynamicState = &dynamicState;

        // Setting up the rasterizer with primitive back face culling
        SetRasterizationState(rasterization, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, 
            VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineInfo.pRasterizationState = &rasterization;

        // No multisampling by default
        SetupMulitsampling(multisampling, VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.f, nullptr, VK_FALSE, VK_FALSE);
        pipelineInfo.pMultisampleState = &multisampling;

        // Depth buffer for reverse z
        //SetupDepthTest(depthState, VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_TRUE, VK_FALSE, 0.f, 0.f, VK_FALSE, 
        //nullptr, nullptr);
        depthState.depthTestEnable = VK_TRUE;
        depthState.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
        depthState.depthWriteEnable = VK_TRUE;
        depthState.stencilTestEnable = VK_FALSE;
        depthState.depthBoundsTestEnable = VK_FALSE;
        pipelineInfo.pDepthStencilState = &depthState;

        // No color blending by default
        CreateColorBlendAttachment(colorBlendAttachment, 
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, 
            VK_FALSE, VK_BLEND_OP_ADD, 
            VK_BLEND_OP_ADD, VK_BLEND_FACTOR_CONSTANT_ALPHA,
            VK_BLEND_FACTOR_CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA, 
            VK_BLEND_FACTOR_CONSTANT_ALPHA);
        CreateColorBlendState(colorBlendState, 1, &colorBlendAttachment, VK_FALSE, VK_LOGIC_OP_AND);
        pipelineInfo.pColorBlendState = &colorBlendState;

        //Setting up the vertex input state (this will not be used but it needs to be passed)
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipelineInfo.pVertexInputState = &vertexInput;
    }

    uint8_t CreateShaderProgram(const VkDevice& device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkShaderModule& shaderModule, VkPipelineShaderStageCreateInfo& pipelineShaderStage, VkSpecializationInfo* pSpecializationInfo /*=nullptr*/)
    {
        // Tries to open the file with the provided path
        BlitzenPlatform::FileHandle handle;
        if (!handle.Open(filepath, BlitzenPlatform::FileModes::Read, 1))
        {
            return 0;
        }
        
        // Reads the shader code in byte format
        size_t filesize = 0;
        BlitCL::String bytes;
        if(!BlitzenPlatform::FilesystemReadAllBytes(handle, bytes, &filesize))
        {
            return 0;
        }

        //Wraps the code in a shader module object
        VkShaderModuleCreateInfo shaderModuleInfo{};
        shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleInfo.codeSize = static_cast<uint32_t>(filesize);
        shaderModuleInfo.pCode = reinterpret_cast<uint32_t*>(bytes.Data());
        if (vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            return 0;
        }

        //Adds a new shader stage based on that shader module
        pipelineShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineShaderStage.module = shaderModule;
        pipelineShaderStage.stage = shaderStage;
        pipelineShaderStage.pName = entryPointName;

        // Adds specialization info if the user requests it (the function assumes that it is properly setup)
        pipelineShaderStage.pSpecializationInfo = pSpecializationInfo;

        return 1;
    }

    uint8_t CreateComputeShaderProgram(VkDevice device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkPipelineLayout& layout, VkPipeline* pPipeline, VkSpecializationInfo* pSpecializationInfo /*=nullptr*/)
    {
        // Creates the shader module and the shader stage
        ShaderModule module{};
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        if (!CreateShaderProgram(device, filepath, VK_SHADER_STAGE_COMPUTE_BIT, entryPointName, module.handle,
            shaderStageInfo, pSpecializationInfo))
        {
            return 0;
        }

        // Sets the pipeline info based on the above
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.flags = 0;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = layout;

        // Creates the compute pipeline
        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, pPipeline) != VK_SUCCESS)
        {
            return 0;
        }
        
        // Success
        return 1;
    }

    void CreateShaderProgramSpecializationConstant(VkSpecializationMapEntry& specializationEntry,
        uint32_t constantId, uint32_t offset, size_t size,
        VkSpecializationInfo& specializationInfo, void* pData)
    {
        specializationEntry.constantID = constantId;
        specializationEntry.offset = offset;
        specializationEntry.size = size;

        specializationInfo.dataSize = size;
        specializationInfo.mapEntryCount = 1;
        specializationInfo.pMapEntries = &specializationEntry;
        specializationInfo.pData = pData;
    }

    VkPipelineInputAssemblyStateCreateInfo SetTriangleListInputAssembly()
    {
        VkPipelineInputAssemblyStateCreateInfo res{};
        res.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        res.flags = 0;
        res.pNext = nullptr;
        res.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        res.primitiveRestartEnable = VK_FALSE;

        return res;
    }

    void SetDynamicStateViewport(VkDynamicState* pStates, VkPipelineViewportStateCreateInfo& viewportState, VkPipelineDynamicStateCreateInfo& dynamicState)
    {
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.scissorCount = 1;
        viewportState.viewportCount = 1;

        pStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
        pStates[1] = VK_DYNAMIC_STATE_SCISSOR;

        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.flags = 0;
        dynamicState.pNext = nullptr;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = pStates;
    }

    void SetRasterizationState(VkPipelineRasterizationStateCreateInfo& rasterization, VkPolygonMode polygonMode, VkCullModeFlags cullMode, 
    VkFrontFace frontFace, VkBool32 depthClampEnable /* = VK_FALSE*/, VkBool32 depthBiasEnable /* = VK_FALSE */)
    {
        rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization.rasterizerDiscardEnable = VK_FALSE;
        rasterization.pNext = nullptr;
        rasterization.flags = 0;
        rasterization.lineWidth = 1.f;

        rasterization.polygonMode = polygonMode;
        rasterization.cullMode = cullMode;
        rasterization.frontFace = frontFace;

        rasterization.depthClampEnable = depthClampEnable;
        rasterization.depthBiasEnable = depthBiasEnable;
    }

    void SetupMulitsampling(VkPipelineMultisampleStateCreateInfo& multisampling, VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples, 
    float minSampleShading, VkSampleMask* pSampleMask, VkBool32 alphaToCoverageEnable, VkBool32 alphaToOneEnable)
    {
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.pNext = nullptr;
        multisampling.sampleShadingEnable = sampleShadingEnable;
        multisampling.rasterizationSamples = rasterizationSamples;
        multisampling.minSampleShading = minSampleShading;
        multisampling.pSampleMask = pSampleMask;
        multisampling.alphaToCoverageEnable = alphaToCoverageEnable;
        multisampling.alphaToOneEnable = alphaToOneEnable;
    }

    void SetupDepthTest(VkPipelineDepthStencilStateCreateInfo& depthState, VkBool32 depthTestEnable, VkCompareOp depthCompareOp, VkBool32 depthWriteEnable, 
    VkBool32 depthBoundsTestEnable, float maxDepthBounds, float minDepthBounds, VkBool32 stencilTestEnable, VkStencilOpState* front, VkStencilOpState* back)
    {
        depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthState.flags = 0;
        depthState.pNext = nullptr;
        depthState.depthTestEnable = depthTestEnable;
        depthState.depthCompareOp = depthCompareOp;
        depthState.depthWriteEnable = depthWriteEnable;
        depthState.depthBoundsTestEnable = depthBoundsTestEnable;
        depthState.maxDepthBounds = maxDepthBounds;
        depthState.minDepthBounds = minDepthBounds;
        depthState.stencilTestEnable = stencilTestEnable;
        if(front)
            depthState.front = *front;
        if(back)
            depthState.back = *back;
    }

    void CreateColorBlendAttachment(VkPipelineColorBlendAttachmentState& colorBlendAttachment, VkColorComponentFlags colorWriteMask, VkBool32 blendEnable, 
    VkBlendOp colorBlendOp, VkBlendOp alphaBlendOp, VkBlendFactor dstAlphaBlendFactor, VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstColorBlendFactor, 
    VkBlendFactor srcColorBlendFactor)
    {
        colorBlendAttachment.colorWriteMask = colorWriteMask;
        colorBlendAttachment.blendEnable = blendEnable;
        colorBlendAttachment.colorBlendOp = colorBlendOp;
        colorBlendAttachment.alphaBlendOp = alphaBlendOp;
        colorBlendAttachment.dstAlphaBlendFactor = dstAlphaBlendFactor;
        colorBlendAttachment.srcAlphaBlendFactor = srcAlphaBlendFactor;
        colorBlendAttachment.dstColorBlendFactor = dstColorBlendFactor;
        colorBlendAttachment.srcColorBlendFactor = srcColorBlendFactor;
    }

    void CreateColorBlendState(VkPipelineColorBlendStateCreateInfo& colorBlending, uint32_t attachmentCount, VkPipelineColorBlendAttachmentState* pAttachments, 
    VkBool32 logicOpEnable, VkLogicOp logicOp)
    {
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.flags = 0;
        colorBlending.pNext = nullptr;
        colorBlending.attachmentCount = attachmentCount;
        colorBlending.pAttachments = pAttachments;
        colorBlending.logicOpEnable = logicOpEnable;
        colorBlending.logicOp = logicOp;
    }

    uint8_t CreatePipelineLayout(VkDevice device, VkPipelineLayout* pLayout, uint32_t descriptorSetLayoutCount, 
    VkDescriptorSetLayout* pDescriptorSetLayouts, uint32_t pushConstantRangeCount, VkPushConstantRange* pPushConstantRanges)
    {
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = descriptorSetLayoutCount;
        layoutInfo.pSetLayouts = pDescriptorSetLayouts;
        layoutInfo.pushConstantRangeCount = pushConstantRangeCount;
        layoutInfo.pPushConstantRanges = pPushConstantRanges;
        VkResult res = vkCreatePipelineLayout(device, &layoutInfo, nullptr, pLayout);
        if(res != VK_SUCCESS)
            return 0;
        return 1;
    }

    void CreateDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding& bindingInfo, uint32_t binding, uint32_t descriptorCount, 
    VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, VkSampler* pImmutableSamplers /*= nullptr*/)
    {
        bindingInfo.binding = binding;
        bindingInfo.descriptorCount = descriptorCount;
        bindingInfo.descriptorType = descriptorType;
        bindingInfo.stageFlags = shaderStage;
        bindingInfo.pImmutableSamplers = pImmutableSamplers;
    }

    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, uint32_t bindingCount, VkDescriptorSetLayoutBinding* pBindings, 
    VkDescriptorSetLayoutCreateFlags flags /*=0*/, void* pNextChain /*=nullptr*/)
    {
        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.flags = flags;
        info.pNext = pNextChain;
        info.bindingCount = bindingCount;
        info.pBindings = pBindings;

        VkDescriptorSetLayout setLayout;
        VkResult res = vkCreateDescriptorSetLayout(device, &info, nullptr, &setLayout);
        if(res != VK_SUCCESS)
            return VK_NULL_HANDLE;
        return setLayout;
    }

    void CreatePushConstantRange(VkPushConstantRange& pushConstant, VkShaderStageFlags shaderStage, uint32_t size, uint32_t offset /* =0 */)
    {
        pushConstant.stageFlags = shaderStage;
        pushConstant.size = size;
        pushConstant.offset = offset;
    }


    uint8_t CreateClusterComputePipelines(VkDevice device, VkPipeline* preClusterPipeline, VkPipeline* initialCullingPipeline,
        VkPipeline* transparentClusterCullPipeline, VkPipelineLayout mainCullingShaderLayout)
    {
        if (!CreateComputeShaderProgram(device, "VulkanShaders/PreClusterDrawCull.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", mainCullingShaderLayout, preClusterPipeline))
        {
            BLIT_ERROR("Failed to create PreClusterDrawCull.comp shader program");
            return 0;
        }

        if (!CreateComputeShaderProgram(device, "VulkanShaders/InitialClusterCull.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", mainCullingShaderLayout, initialCullingPipeline))
        {
            BLIT_ERROR("Failed to create InitialClusterCull.comp shader program");
            return 0;
        }

        if (!CreateComputeShaderProgram(device, "VulkanShaders/TransparentClusterCull.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", mainCullingShaderLayout, transparentClusterCullPipeline))
        {
			BLIT_ERROR("Failed to create transparentClusterCull.comp shader program");
            return 0;
        }

		// Success
        return 1;
    }


    uint8_t CreateComputeShaders(VkDevice device, VkPipeline* cullingPipeline, VkPipeline* lateCullingPipeline, 
        VkPipeline* onpcCullPipeline, VkPipeline* transparentCullShaderPipeline, VkPipelineLayout mainCullingShaderLayout, 
        VkPipeline* depthPyramidGenerationPipeline, VkPipelineLayout depthPyramidGenerationLayout, 
        VkPipeline* presentImageGenerationPipeline, VkPipelineLayout presentImageGenerationPipelineLayout)
    {

        if (!CreateComputeShaderProgram(device, "VulkanShaders/InitialDrawCull.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", mainCullingShaderLayout, cullingPipeline))
        {
            BLIT_ERROR("Failed to create InitialDrawCull.comp shader program");
            return 0;
        }

        // Late culling shader compute pipeline
        if (!CreateComputeShaderProgram(device, "VulkanShaders/LateDrawCull.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", mainCullingShaderLayout, lateCullingPipeline))
        {
            BLIT_ERROR("Failed to create LateDrawCull.comp shader program");
            return 0;
        }

        // Generate depth pyramid compute shader
        if (!CreateComputeShaderProgram(device, "VulkanShaders/DepthPyramidGeneration.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", depthPyramidGenerationLayout, depthPyramidGenerationPipeline))
        {
            BLIT_ERROR("Failed to create DepthPyramidGeneration.comp shader program");
            return 0;
        }

        // Redundant shader
        if (!CreateComputeShaderProgram(device, "VulkanShaders/OnpcDrawCull.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", mainCullingShaderLayout, onpcCullPipeline))
        {
            BLIT_ERROR("Failed to create OnpcDrawCull.comp shader program");
            return 0;
        }

        if (!CreateComputeShaderProgram(device, "VulkanShaders/TransparentDrawCull.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", mainCullingShaderLayout, transparentCullShaderPipeline))
        {
            BLIT_ERROR("Failed to create OnpcDrawCull.comp shader program");
            return 0;
        }

        // Creates the generate presentation image compute shader program
        if (!CreateComputeShaderProgram(device, "VulkanShaders/GeneratePresentation.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", presentImageGenerationPipelineLayout, presentImageGenerationPipeline))
        {
            BLIT_ERROR("Failed to create GeneratePresentation.comp shader program");
            return 0;
        }

        // Success
        return 1;
    }

    static uint8_t CreateGraphicsPipelineWithShader(VkDevice device, VkPipelineLayout layout, VkPipeline* pPipeline, 
        uint32_t shaderStageCount, VkPipelineShaderStageCreateInfo* pShaderStages)
    {
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        VkPipelineRenderingCreateInfo dynamicRenderingInfo{};
        VkFormat colorAttachmentFormat = ce_colorAttachmentFormat;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        VkPipelineViewportStateCreateInfo viewport{};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        VkPipelineRasterizationStateCreateInfo rasterization{};
        VkPipelineMultisampleStateCreateInfo multisampling{};
        VkPipelineDepthStencilStateCreateInfo depthState{};
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        VkDynamicState dynamicStates[2] = {};
        GetDefaultPipelineInfo(pipelineInfo, dynamicRenderingInfo, &colorAttachmentFormat,
            inputAssembly, viewport, dynamicState, rasterization, multisampling, depthState, colorBlendAttachment,
            colorBlendState, vertexInput, dynamicStates);
        // Main graphics pipeline
        pipelineInfo.stageCount = shaderStageCount;
        pipelineInfo.pStages = pShaderStages;
        pipelineInfo.layout = layout;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
            pPipeline) != VK_SUCCESS)
        {
            return 0;
        }

        // Success 
        return 1;
    }

    uint8_t CreateGraphicsPipelines(VkDevice device, uint8_t bMeshShaders, 
        VkPipeline* mainGraphicsPipeline, VkPipeline* postPassGraphicsPipeline, VkPipelineLayout mainGraphicsPipelineLayout, 
        VkPipeline* onpcPipeline, VkPipelineLayout onpcPipelineLayout)
    {
        // Main(opaque) graphics pipeline
        ShaderModule vertexShaderModule;
        ShaderModule taskShaderModule;
        VkPipelineShaderStageCreateInfo shaderStages[3] = {};
        if(bMeshShaders)
        {
            if (!CreateShaderProgram(device, "VulkanShaders/MeshShader.mesh.glsl.spv",
                VK_SHADER_STAGE_MESH_BIT_EXT, "main", vertexShaderModule.handle, shaderStages[0]))
            {
                BLIT_ERROR("Failed to create MeshShader.mesh shader program");
                return 0;
            }

            if (!CreateShaderProgram(device, "VulkanShaders/MeshShader.task.glsl.spv",
                VK_SHADER_STAGE_TASK_BIT_EXT, "main", taskShaderModule.handle, shaderStages[2]))
            {
                BLIT_ERROR("Failed to create MeshShader.task shader program");
                return 0;
            }
        }
        else
        {
            // Vertex shader for traditional pipeline
            if (!CreateShaderProgram(device, "VulkanShaders/MainObjectShader.vert.glsl.spv",
                VK_SHADER_STAGE_VERTEX_BIT, "main", vertexShaderModule.handle, shaderStages[0]))
            {
                BLIT_ERROR("Failed to create MainObjectShader.vert shader program");
                return 0;
            }
        }
        ShaderModule fragShaderModule;
        if (!CreateShaderProgram(device, "VulkanShaders/MainObjectShader.frag.glsl.spv",
            VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragShaderModule.handle, shaderStages[1]))
        {
            BLIT_ERROR("Failed to create MainObjectShader.frag shader program");
            return 0;
        }
		if (!CreateGraphicsPipelineWithShader(device, mainGraphicsPipelineLayout, mainGraphicsPipeline, 
            bMeshShaders ? 3 : 2, shaderStages))
		{
			BLIT_ERROR("Failed to create main graphics pipeline");
			return 0;
		}

        // Tranparent pipeline specialization
		VkPipelineShaderStageCreateInfo postPassShaderStages[2] = {};
		postPassShaderStages[0] = shaderStages[0];
        VkSpecializationMapEntry postPassSpecializationMapEntry{};
        VkSpecializationInfo postPassSpecialization{};
        uint32_t postPass = 1;
        CreateShaderProgramSpecializationConstant(postPassSpecializationMapEntry,
            0, 0, sizeof(uint32_t), postPassSpecialization, &postPass);
        ShaderModule postPassFragShaderModule;
        if (!CreateShaderProgram(device, "VulkanShaders/MainObjectShader.frag.glsl.spv",
            VK_SHADER_STAGE_FRAGMENT_BIT, "main", postPassFragShaderModule.handle,
            postPassShaderStages[1], &postPassSpecialization))
        {
            BLIT_ERROR("Failed to create MainObjectShader.frag post pass specialization shader program");
            return 0;
        }
        if (!CreateGraphicsPipelineWithShader(device, mainGraphicsPipelineLayout, postPassGraphicsPipeline, 
            BLIT_ARRAY_SIZE(postPassShaderStages), postPassShaderStages))
        {
            BLIT_ERROR("Failed to create post pass graphics pipeline")
            return 0;
        }
        
        // This was for a temporary math test, I will keep it for some time
        ShaderModule onpcVertexShaderModule;
        VkPipelineShaderStageCreateInfo onpcGeometryShaderStages[2] = {};
        if (!CreateShaderProgram(device, "VulkanShaders/OnpcGeometry.vert.glsl.spv",
            VK_SHADER_STAGE_VERTEX_BIT, "main", onpcVertexShaderModule.handle, onpcGeometryShaderStages[0]))
        {
            BLIT_ERROR("Failed to create OnpcGeometry.vert shader program");
            return 0;
        }
        onpcGeometryShaderStages[1] = shaderStages[1];
        if (!CreateGraphicsPipelineWithShader(device, onpcPipelineLayout, onpcPipeline, 
            BLIT_ARRAY_SIZE(onpcGeometryShaderStages), onpcGeometryShaderStages))
        {
			BLIT_ERROR("Failed to create onpc graphics pipeline");
            return 0;
        }

        // Success
        return 1;
    }

    uint8_t CreateLoadingTrianglePipeline(VkDevice device, VkPipeline& pipeline, VkPipelineLayout& layout)
    {
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        VkPipelineRenderingCreateInfo dynamicRenderingInfo{};
        VkFormat colorAttachmentFormat = VK_FORMAT_B8G8R8A8_UNORM;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        VkPipelineViewportStateCreateInfo viewport{};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        VkPipelineRasterizationStateCreateInfo rasterization{};
        VkPipelineMultisampleStateCreateInfo multisampling{};
        VkPipelineDepthStencilStateCreateInfo depthState{};
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        VkDynamicState dynamicStates[2];
        GetDefaultPipelineInfo(pipelineInfo, dynamicRenderingInfo, &colorAttachmentFormat,
            inputAssembly, viewport, dynamicState, rasterization, multisampling, depthState, colorBlendAttachment,
            colorBlendState, vertexInput, dynamicStates);

        ShaderModule vertexShaderModule;
        VkPipelineShaderStageCreateInfo shaderStages[2] = {};
        if (!CreateShaderProgram(device, "VulkanShaders/LoadingTriangle.vert.glsl.spv",
            VK_SHADER_STAGE_VERTEX_BIT, "main", vertexShaderModule.handle, shaderStages[0]))
        {
            BLIT_ERROR("Failed to create idle draw fragment shader program");
            return 0;
        }

        // Fragment shader is common
        ShaderModule fragShaderModule;
        if (!CreateShaderProgram(device, "VulkanShaders/LoadingTriangle.frag.glsl.spv",
            VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragShaderModule.handle, shaderStages[1]))
        {
            BLIT_ERROR("Failed to create idle draw fragment shader program");
            return 0;
        }

        // mesh pipeline has additional task shader
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        VkPushConstantRange pushConstant{};
		CreatePushConstantRange(pushConstant, VK_SHADER_STAGE_VERTEX_BIT, sizeof(BlitML::vec3));
        if (!CreatePipelineLayout(device, &layout,
            0, nullptr, 1, &pushConstant))
        {
            BLIT_ERROR("Failed to create idle draw pipeline layout");
            return 0;
        }

        //Create the graphics pipeline
        pipelineInfo.layout = layout;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, Ce_SinglePointer, &pipelineInfo,
            nullptr, &pipeline) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create idle draw pipeline");
            return 0;
        }

        return 1;
    }
}