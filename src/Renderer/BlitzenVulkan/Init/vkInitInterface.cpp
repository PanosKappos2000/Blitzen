#include "Renderer/Interface/blitRenderer.h"
#include "vulkanInit.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanRNDResources.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanPipelines.h"

namespace BlitzenEngine
{
    uint8_t StartupRenderer(BlitzenVulkan::VulkanRenderer* pRenderer, uint32_t windowWidth, uint32_t windowHeight, void* pPlatform)
    {

        if (!BlitzenVulkan::CreateInstance(pRenderer->m_instance, pRenderer->m_stats, &pRenderer->m_debugMessenger))
        {
            BLIT_ERROR("%s: Failed to create vulkan instance", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenPlatform::CreateVulkanSurface(pRenderer->m_instance, pRenderer->m_surface.handle, nullptr, pPlatform))
        {
            BLIT_ERROR("%s: Failed to create Vulkan window surface", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenVulkan::PickPhysicalDevice(pRenderer->m_physicalDevice, pRenderer->m_instance, pRenderer->m_surface.handle, pRenderer->m_graphicsQueue, 
            pRenderer->m_computeQueue, pRenderer->m_presentQueue, pRenderer->m_transferQueue, pRenderer->m_stats))
        {
            BLIT_ERROR("%s: Failed to pick suitable physical device", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!CreateDevice(pRenderer->m_device, pRenderer->m_physicalDevice, pRenderer->m_graphicsQueue, pRenderer->m_presentQueue, pRenderer->m_computeQueue, pRenderer->m_transferQueue, 
            pRenderer->m_stats))
        {
            BLIT_ERROR("%s: Failed to pick suitable physical device", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenVulkan::CreateSwapchain(pRenderer->m_device, pRenderer->m_surface.handle, pRenderer->m_physicalDevice, windowWidth, windowHeight, pRenderer->m_graphicsQueue, 
            pRenderer->m_presentQueue, pRenderer->m_computeQueue, nullptr, pRenderer->m_swapchain, VK_NULL_HANDLE))
        {
            BLIT_ERROR("%s: Failed to create Vulkan swapchain", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        // Commands
        for (size_t frame = 0; frame < BlitzenVulkan::ce_framesInFlight; ++frame)
        {
            if (!pRenderer->m_commandsContext[frame].Init(pRenderer->m_device, pRenderer->m_graphicsQueue, pRenderer->m_transferQueue, pRenderer->m_computeQueue))
            {
                BLIT_ERROR("%s: Failed to create command handles", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
                return 0;
            }
        }

        // This will be referred to by rendering attachments and will be updated when the window is resized
        pRenderer->m_drawWidth = pRenderer->m_swapchain.m_extent.width;
        pRenderer->m_drawHeight = pRenderer->m_swapchain.m_extent.height;

        // Resource management
        if (!BlitzenVulkan::SetupResourceManagement(pRenderer->m_device, pRenderer->m_physicalDevice, pRenderer->m_instance, pRenderer->m_allocator, pRenderer->m_memoryCrucials))
        {
            BLIT_ERROR("%s: Failed to startup vulkan resources management", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        auto pMemory{ BlitzenVulkan::InitMemoryCrucialHandles(&pRenderer->m_memoryCrucials) };
        if (!pMemory)
        {
            BLIT_ERROR("%s: Failed to load vulkan memory crucial handles", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        for (uint32_t frame = 0; frame < BlitzenVulkan::ce_framesInFlight; ++frame)
        {
            if (!BlitzenVulkan::RenderingAttachmentsInit(pRenderer->m_device, pRenderer->m_allocator, pRenderer->m_readOnlies, pRenderer->m_readWrites[frame], 
                pRenderer->m_descriptorContext, pRenderer->m_pipelines, pRenderer->m_drawWidth, pRenderer->m_drawHeight, frame))
            {
                BLIT_ERROR("%s: Failed to create rendering attachments", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
                return 0;
            }
        }

        if (!BlitzenVulkan::CreateDescriptorLayouts(pRenderer->m_device, pRenderer->m_descriptorContext, pRenderer->m_stats, 0))
        {
            BLIT_ERROR("%s: Failed to create descriptor set layouts", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if(!BlitzenVulkan::CreatePipelineLayouts(pRenderer->m_device, pRenderer->m_pipelines, pRenderer->m_descriptorContext))
        {
            BLIT_ERROR("%s: Failed to create pipeline layouts", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenVulkan::CreateIdleDrawHandles(pRenderer->m_device, pRenderer->m_pipelines, pRenderer->m_descriptorContext.m_backgroundSetLayout.handle, pRenderer->m_graphicsQueue.index))
        {
            BLIT_ERROR("%s: Failed to create idle draw handles", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenVulkan::CreateLoadingTrianglePipeline(pRenderer->m_device, pRenderer->m_pipelines))
        {
            BLIT_ERROR("%s: Failed to create loading triangle pipeline", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenVulkan::CreateComputeShaders(pRenderer->m_device, pRenderer->m_pipelines))
        {
            BLIT_ERROR("%s: Failed to create compute shaders", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        // Create the graphics pipeline object 
        if (!BlitzenVulkan::CreateGraphicsPipelines(pRenderer->m_device, pRenderer->m_stats.meshShaderSupport, pRenderer->m_pipelines))
        {
            BLIT_ERROR("%s: Failed to create graphics pipelines", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        // Texture sampler. Global for all textures for now
        pRenderer->m_readOnlies.m_textureSampler.m_handle = BlitzenVulkan::CreateSampler(pRenderer->m_device, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
        if (pRenderer->m_readOnlies.m_textureSampler.m_handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("%s: Failed to create texture sampler", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenVulkan::CreateReadWriteBuffers(pRenderer->m_device, pRenderer->m_allocator, pRenderer->m_readWrites, pRenderer->m_descriptorContext))
        {
            BLIT_ERROR("%s: Failed to create read write resources", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenVulkan::CreateReadOnlyBuffers(pRenderer->m_device, pRenderer->m_allocator, pRenderer->m_readOnlies, pRenderer->m_stats))
        {
            BLIT_ERROR("%s: Failed to create read only resources", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        // Success
        return 1;
    }
}