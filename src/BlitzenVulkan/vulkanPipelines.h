#pragma once
#include "vulkanData.h"

namespace BlitzenVulkan
{
    void GetDefaultPipelineInfo(VkGraphicsPipelineCreateInfo& pipelineInfo,
        VkPipelineRenderingCreateInfo dynamicRenderingInfo, VkFormat* pFormat,
        VkPipelineInputAssemblyStateCreateInfo& inputAssembly,
        VkPipelineViewportStateCreateInfo& viewport,
        VkPipelineDynamicStateCreateInfo& dynamicState,
        VkPipelineRasterizationStateCreateInfo& rasterization,
        VkPipelineMultisampleStateCreateInfo& multisampling,
        VkPipelineDepthStencilStateCreateInfo& depthState,
        VkPipelineColorBlendAttachmentState& colorBlendAttachment,
        VkPipelineColorBlendStateCreateInfo& colorBlendState,
        VkPipelineVertexInputStateCreateInfo vertexInput, 
        VkDynamicState* pDynamicStates
    );

    // Creates a shader stage with a given module handle and a filepath to a shader
    uint8_t CreateShaderProgram(const VkDevice& device, const char* filepath, 
        VkShaderStageFlagBits shaderStage, const char* entryPointName, 
        VkShaderModule& shaderModule, VkPipelineShaderStageCreateInfo& pipelineShaderStage, 
        VkSpecializationInfo* pSpecializationInfo = nullptr
    );

    // Tries to create a compute pipeline
    uint8_t CreateComputeShaderProgram(VkDevice device, const char* filepath, 
        VkShaderStageFlagBits shaderStage, const char* entryPointName, 
        VkPipelineLayout& layout, VkPipeline* pPipeline, 
        VkSpecializationInfo* pSpecializationInfo = nullptr
    );

    // Initializes the given specialization info with a single given specialization entry
    void CreateShaderProgramSpecializationConstant(VkSpecializationMapEntry& specializationEntry,
        uint32_t constantId, uint32_t offset, size_t size,
        VkSpecializationInfo& specializationInfo, void* pData
    );

    VkPipelineInputAssemblyStateCreateInfo SetTriangleListInputAssembly();

    void SetDynamicStateViewport(VkDynamicState* pStates, 
        VkPipelineViewportStateCreateInfo& viewportState, 
        VkPipelineDynamicStateCreateInfo& dynamicState
    );

    void SetRasterizationState(VkPipelineRasterizationStateCreateInfo& rasterization, 
        VkPolygonMode polygonMode, VkCullModeFlags cullMode, 
        VkFrontFace frontFace, VkBool32 depthClampEnable = VK_FALSE, 
        VkBool32 depthBiasEnable = VK_FALSE);

    void SetupMulitsampling(VkPipelineMultisampleStateCreateInfo& multisampling, 
        VkBool32 sampleShadingEnable, VkSampleCountFlagBits rasterizationSamples, 
        float minSampleShading, VkSampleMask* pSampleMask, VkBool32 alphaToCoverageEnable, 
        VkBool32 alphaToOneEnable
    );

    void SetupDepthTest(VkPipelineDepthStencilStateCreateInfo& depthState, VkBool32 depthTestEnable, 
        VkCompareOp depthCompareOp, VkBool32 depthWriteEnable, 
        VkBool32 depthBoundsTestEnable, float maxDepthBounds, float minDepthBounds, 
        VkBool32 stencilTestEnable, VkStencilOpState* front, VkStencilOpState* back
    );

    void CreateColorBlendAttachment(VkPipelineColorBlendAttachmentState& colorBlendAttachment, 
        VkColorComponentFlags colorWriteMask, VkBool32 blendEnable, 
        VkBlendOp colorBlendOp, VkBlendOp alphaBlendOp, VkBlendFactor dstAlphaBlendFactor, 
        VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstColorBlendFactor, 
        VkBlendFactor srcColorBlendFactor
    );

    void CreateColorBlendState(VkPipelineColorBlendStateCreateInfo& colorBlending, 
        uint32_t attachmentCount, VkPipelineColorBlendAttachmentState* pAttachments, 
        VkBool32 logicOpEnable, VkLogicOp logicOp
    );

    //Helper function that creates a pipeline layout that will be used by a compute or graphics pipeline
    uint8_t CreatePipelineLayout(VkDevice device, VkPipelineLayout* layout, 
        uint32_t descriptorSetLayoutCount, VkDescriptorSetLayout* pDescriptorSetLayouts, 
        uint32_t pushConstantRangeCount, VkPushConstantRange* pPushConstantRanges
    );

    //Helper function for pipeline layout creation, takes care of a single set layout binding creation
    void CreateDescriptorSetLayoutBinding(VkDescriptorSetLayoutBinding& bindingInfo, 
        uint32_t binding, uint32_t descriptorCount, 
        VkDescriptorType descriptorType, VkShaderStageFlags shaderStage, 
        VkSampler* pImmutableSamplers = nullptr
    );

    // Initializes the member of a VkDescriptorSetLayoutCreateInfo instance and calls vkCreateDescriptorSetLayout
    // Returns the resulting VkDescriptorSetLayout, return VK_NULL_HANDLE if it fails
    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, 
        uint32_t bindingCount, VkDescriptorSetLayoutBinding* pBindings, 
        VkDescriptorSetLayoutCreateFlags flags = 0, void* pNextChain = nullptr
    );

    //Helper function for pipeline layout creation, takes care of a single push constant creation
    void CreatePushConstantRange(VkPushConstantRange& pushConstant, VkShaderStageFlags shaderStage, 
        uint32_t size, uint32_t offset = 0
    );
}