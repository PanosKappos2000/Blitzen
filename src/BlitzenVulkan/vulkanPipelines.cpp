#include "vulkanRenderer.h"
// Temporary, I will like to handle this myself
#include <fstream>

#include "Platform/filesystem.h"

namespace BlitzenVulkan
{
    void CreateShaderProgram(const VkDevice& device, const char* filepath, VkShaderStageFlagBits shaderStage, const char* entryPointName, 
    VkShaderModule& shaderModule, VkPipelineShaderStageCreateInfo& pipelineShaderStage, BlitCL::DynamicArray<char>* shaderCode)
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
        vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule);

        //Adds a new shader stage based on that shader module
        pipelineShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineShaderStage.module = shaderModule;
        pipelineShaderStage.stage = shaderStage;
        pipelineShaderStage.pName = entryPointName;
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

    void SetDynamicStateViewport(VkDynamicState* pStates, VkPipelineViewportStateCreateInfo& viewportState)
    {
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.scissorCount = 1;
        viewportState.viewportCount = 1;

        pStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
        pStates[1] = VK_DYNAMIC_STATE_SCISSOR;
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
    VkBool32 depthBoundsTestEnable, float maxDepthBounds, float minDepthBounds, VkBool32 stencilTestEnable, VkStencilOpState front, VkStencilOpState back)
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
        depthState.front = front;
        depthState.back = back;
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
        vkCreatePipelineLayout(device, &layoutInfo, nullptr, pLayout);
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

    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, uint32_t bindingCount, VkDescriptorSetLayoutBinding* pBindings)
    {
        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = bindingCount;
        info.pBindings = pBindings;

        VkDescriptorSetLayout setLayout;
        vkCreateDescriptorSetLayout(device, &info, nullptr, &setLayout);
        return setLayout;
    }

    void CreatePushConstantRange(VkPushConstantRange& pushConstant, VkShaderStageFlags shaderStage, uint32_t size, uint32_t offset /* =0 */)
    {
        pushConstant.stageFlags = shaderStage;
        pushConstant.size = size;
        pushConstant.offset = offset;
    }
}