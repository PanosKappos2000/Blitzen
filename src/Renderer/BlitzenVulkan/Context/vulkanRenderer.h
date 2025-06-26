#pragma once
#include "vulkanContext.h"
#include "Renderer/Interface/blitRendererInterface.h"
#include "BlitCL/blitDynamicArr.h"

namespace BlitzenVulkan
{
    struct VulkanStats
    {
        uint8_t hasDiscreteGPU = 0;

        uint8_t meshShaderSupport = 0;

        uint8_t bSynchronizationValidationSupported = 0;

        uint8_t bRayTracingSupported = 0;

        BlitCL::DynamicArray<const char*> m_instExtensions;
        BlitCL::DynamicArray<const char*> m_dvExtensions;
    };

    class VulkanRenderer
    {

    public:

        VulkanRenderer() = default;
        VulkanRenderer operator = (const VulkanRenderer& vk) = delete;
        VulkanRenderer(const VulkanRenderer& vk) = delete;

        ~VulkanRenderer();

        uint8_t UploadTexture(const char* filepath);

        void DrawWhileWaiting(float deltaTime);

        void CopyTargetToSwapchain(VkCommandBuffer commandBuffer);

        void LendRenderingInfos(VkRenderingAttachmentInfo** ppColorInfo, VkImage* pColorTarget);

    public:

        uint8_t m_currentFrame{ 0 };
        uint32_t m_swapchainIDX{ 0 };

        uint32_t m_drawWidth;
        uint32_t m_drawHeight;

        MemoryCrucialHandles m_memoryCrucials;

        // Vulkan API and memory crucials
        VkInstance m_instance;
        VkPhysicalDevice m_physicalDevice;
        VkDevice m_device;

        SurfaceKHR m_surface;
        Swapchain m_swapchain;

        VmaAllocator m_allocator;

        VulkanStats m_stats;

        Queue m_graphicsQueue;
        Queue m_presentQueue;
        Queue m_computeQueue;
        Queue m_transferQueue;

        CommandContext m_commandsContext[ce_framesInFlight];
        VkSemaphore m_secondWaitSemaphore{ VK_NULL_HANDLE };

        VkDebugUtilsMessengerEXT m_debugMessenger;

        ROResources m_readOnlies;

        RWResources m_readWrites[ce_framesInFlight];

        DescriptorContext m_descriptorContext;

        PipelineContext m_pipelines;
    };
    

    uint8_t BuildBlas(VkInstance instance, VkDevice device, VmaAllocator vma, CommandContext& commands, VkQueue queue, BlitzenEngine::DrawContext& context, 
        ROResources& readOnlies);

    uint8_t BuildTlas(VkInstance instance, VkDevice device, VmaAllocator vma, CommandContext& commands, VkQueue queue, ROResources& readOnlies,
        BlitzenEngine::DrawContext& context);
}