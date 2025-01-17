#include "vulkanRenderer.h"
// Temporary, I will like to handle this myself
#include <fstream>

#include "Platform/filesystem.h"

namespace BlitzenVulkan
{
    void CreateShaderProgram(const VkDevice& device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkShaderModule& shaderModule, VkPipelineShaderStageCreateInfo& pipelineShaderStage)
    {
        BlitzenPlatform::FileHandle handle;
        // If the file did not open, something might be wrong with the filepath, so it needs to be checked
        BLIT_ASSERT(BlitzenPlatform::OpenFile(filepath, BlitzenPlatform::FileModes::Read, 1, handle))
        size_t filesize = 0;
        uint8_t* pBytes = nullptr;
        BLIT_ASSERT(BlitzenPlatform::FilesystemReadAllBytes(handle, &pBytes, &filesize))
        BlitzenPlatform::CloseFile(handle);

        //Wrap the code in a shader module object
        VkShaderModuleCreateInfo shaderModuleInfo{};
        shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleInfo.codeSize = static_cast<uint32_t>(filesize);
        shaderModuleInfo.pCode = reinterpret_cast<uint32_t*>(pBytes);
        VK_CHECK(vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule));

        //Adds a new shader stage based on that shader module
        pipelineShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineShaderStage.module = shaderModule;
        pipelineShaderStage.stage = shaderStage;
        pipelineShaderStage.pName = entryPointName;
    }

    void CreateComputeShaderProgram(VkDevice device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkPipelineLayout& layout, VkPipeline* pPipeline)
    {
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.flags = 0;
        pipelineInfo.pNext = nullptr;

        VkShaderModule computeShaderModule{};
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        CreateShaderProgram(device, filepath, VK_SHADER_STAGE_COMPUTE_BIT, entryPointName, computeShaderModule, 
        shaderStageInfo);

        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = layout;

        VK_CHECK(vkCreateComputePipelines(device, nullptr, 1, &pipelineInfo, nullptr, pPipeline))

        // Beyond this scope, this shader module is not needed
        vkDestroyShaderModule(device, computeShaderModule, nullptr);
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

    void CreatePipelineLayout(VkDevice device, VkPipelineLayout* pLayout, uint32_t descriptorSetLayoutCount, 
    VkDescriptorSetLayout* pDescriptorSetLayouts, uint32_t pushConstantRangeCount, VkPushConstantRange* pPushConstantRanges)
    {
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = descriptorSetLayoutCount;
        layoutInfo.pSetLayouts = pDescriptorSetLayouts;
        layoutInfo.pushConstantRangeCount = pushConstantRangeCount;
        layoutInfo.pPushConstantRanges = pPushConstantRanges;
        VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, pLayout));
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
    VkDescriptorSetLayoutCreateFlags flags /* = 0 */)
    {
        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.flags = flags;
        info.pNext = nullptr;
        info.bindingCount = bindingCount;
        info.pBindings = pBindings;

        VkDescriptorSetLayout setLayout;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &setLayout));
        return setLayout;
    }

    void CreatePushConstantRange(VkPushConstantRange& pushConstant, VkShaderStageFlags shaderStage, uint32_t size, uint32_t offset /* =0 */)
    {
        pushConstant.stageFlags = shaderStage;
        pushConstant.size = size;
        pushConstant.offset = offset;
    }




    void VulkanRenderer::SetupMainGraphicsPipeline()
    {
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.flags = 0;
        pipelineInfo.renderPass = VK_NULL_HANDLE; // Using dynamic rendering
        pipelineInfo.layout = m_opaqueGraphicsPipelineLayout;// The layout is expected to have been created earlier

        // Setting up dynamic rendering info for graphics pipeline
        VkPipelineRenderingCreateInfo dynamicRenderingInfo{};
        dynamicRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        VkFormat colorAttachmentFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        dynamicRenderingInfo.colorAttachmentCount = 1;
        dynamicRenderingInfo.pColorAttachmentFormats = &colorAttachmentFormat;
        dynamicRenderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
        pipelineInfo.pNext = &dynamicRenderingInfo;

        // Loading the vertex shader code (or mesh shading shader if mesh shading is used)
        VkShaderModule vertexShaderModule;
        VkPipelineShaderStageCreateInfo shaderStages[3] = {};
        // Create the mesh shader program for vertex shader if mesh shaders are requested and supported
        if(m_stats.meshShaderSupport)
        {
            CreateShaderProgram(m_device, "VulkanShaders/MeshShader.mesh.glsl.spv", VK_SHADER_STAGE_MESH_BIT_EXT, "main", vertexShaderModule, 
            shaderStages[0]);
        }
        // Create the vertex shader program for vertex processing if mesh shaders were not requested or not supported
        else
        {
            CreateShaderProgram(m_device, "VulkanShaders/MainObjectShader.vert.glsl.spv", VK_SHADER_STAGE_VERTEX_BIT, "main", vertexShaderModule,
            shaderStages[0]);
        }

        //Loading the fragment shader
        VkShaderModule fragShaderModule;
        CreateShaderProgram(m_device, "VulkanShaders/MainObjectShader.frag.glsl.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragShaderModule, 
        shaderStages[1]);

        // Loading the task shader if mesh shaders are requested and supported
        VkShaderModule taskShaderModule;
        if(m_stats.meshShaderSupport)
        {
            CreateShaderProgram(m_device, "VulkanShaders/MeshShader.task.glsl.spv", VK_SHADER_STAGE_TASK_BIT_EXT, "main", taskShaderModule, 
            shaderStages[2]);
        }

        // If the pipeline is going to use mesh shaders, the task shader needs to also be supported, so there will be 3 shader stages
        if(m_stats.meshShaderSupport)
        {
            pipelineInfo.stageCount = 3;
        }
        else
        {
            pipelineInfo.stageCount = 2;
        }
        pipelineInfo.pStages = shaderStages;

        // Setting up triangle primitive assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = SetTriangleListInputAssembly();
        pipelineInfo.pInputAssemblyState = &inputAssembly;

        // Setting the viewport and scissor as dynamic states
        VkDynamicState dynamicStates[2];
        VkPipelineViewportStateCreateInfo viewport{};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        SetDynamicStateViewport(dynamicStates, viewport, dynamicState);
        pipelineInfo.pViewportState = &viewport;
        pipelineInfo.pDynamicState = &dynamicState;

        // Setting up the rasterizer with primitive back face culling
        VkPipelineRasterizationStateCreateInfo rasterization{};
        SetRasterizationState(rasterization, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineInfo.pRasterizationState = &rasterization;

        // Setting up multisampling(no multisampling for now)
        VkPipelineMultisampleStateCreateInfo multisampling{};
        SetupMulitsampling(multisampling, VK_FALSE, VK_SAMPLE_COUNT_1_BIT, 1.f, nullptr, VK_FALSE, VK_FALSE);
        pipelineInfo.pMultisampleState = &multisampling;

        // Depth buffer for reverse z
        VkPipelineDepthStencilStateCreateInfo depthState{};
        SetupDepthTest(depthState, VK_TRUE, VK_COMPARE_OP_GREATER, VK_TRUE, VK_FALSE, 0.f, 0.f, VK_FALSE, 
        nullptr, nullptr);
        pipelineInfo.pDepthStencilState = &depthState;

        // Color blending. This pipeline is for opaque objects so it is left as default
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        CreateColorBlendAttachment(colorBlendAttachment, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE, VK_BLEND_OP_ADD, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_CONSTANT_ALPHA, 
        VK_BLEND_FACTOR_CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA);
        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        CreateColorBlendState(colorBlendState, 1, &colorBlendAttachment, VK_FALSE, VK_LOGIC_OP_AND);
        pipelineInfo.pColorBlendState = &colorBlendState;

        //Setting up the vertex input state (this will not be used but it needs to be passed)
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        pipelineInfo.pVertexInputState = &vertexInput;

        //Create the graphics pipeline
        VK_CHECK(vkCreateGraphicsPipelines(m_device, nullptr, 1, &pipelineInfo, m_pCustomAllocator, &m_opaqueGraphicsPipeline))

        // Destroy the shader modules after pipeline has been created
        vkDestroyShaderModule(m_device, vertexShaderModule, m_pCustomAllocator);
        vkDestroyShaderModule(m_device, fragShaderModule, m_pCustomAllocator);
        if(m_stats.meshShaderSupport)
        {
            vkDestroyShaderModule(m_device, taskShaderModule, m_pCustomAllocator);
        }
    }
}