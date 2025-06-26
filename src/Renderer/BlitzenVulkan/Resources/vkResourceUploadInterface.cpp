#include "Renderer/Interface/blitRenderer.h"
#include "vkResourcesUpload.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
	uint8_t UploadResourcesToGPU(BlitzenVulkan::VulkanRenderer* pRenderer, BlitzenEngine::DrawContext& drawContext)
	{
        if (!BlitzenVulkan::UploadResourcesToBuffers(pRenderer->m_device, pRenderer->m_instance, pRenderer->m_allocator, pRenderer->m_transferQueue.handle, drawContext, pRenderer->m_readOnlies, 
            pRenderer->m_readWrites, pRenderer->m_commandsContext[0], pRenderer->m_stats))
        {
            BLIT_ERROR("%s: Failed to upload data to buffers", BlitzenCore::CE_VULKAN_SYSTEM_NAME);
            return 0;
        }

        if (!BlitzenVulkan::AllocateTextureDescriptorSet(pRenderer->m_device, pRenderer->m_readOnlies, pRenderer->m_descriptorContext))
        {
            BLIT_ERROR("Failed to allocate texture descriptor sets");
            return 0;
        }

        BlitzenVulkan::CreateDescriptors(pRenderer->m_descriptorContext, pRenderer->m_readOnlies, pRenderer->m_readWrites, drawContext);

        // Updates the reference to the depth pyramid width held by the camera
        drawContext.m_camera.viewData.pyramidWidth = float(pRenderer->m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_width);
        drawContext.m_camera.viewData.pyramidHeight = float(pRenderer->m_readWrites[0].m_HI_Z_MAP.m_pyramid.m_height);

        return 1;
	}
}