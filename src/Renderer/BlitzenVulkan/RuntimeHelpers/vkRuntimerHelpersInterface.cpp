#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"
#include "vulkanCommands.h" 
#include "BlitCL/blitDynamicArr.h"

namespace BlitzenEngine
{
    void PrepareRendererForRuntime(BlitzenVulkan::VulkanRenderer* pRenderer)
    {
        vkDeviceWaitIdle(pRenderer->m_device);

		uint32_t dummyTextureOffset = pRenderer->m_readOnlies.m_textureCount;
        BlitCL::DynamicArray<VkImageMemoryBarrier2> dymmyTextureMemoryBarriers{ BlitzenCore::Ce_MaxTextureCount, VkImageMemoryBarrier2{} };

        for (uint32_t tex = dummyTextureOffset; tex < BlitzenCore::Ce_MaxTextureCount; ++tex)
        {
            BlitzenVulkan::ImageMemoryBarrier(pRenderer->m_readOnlies.m_textures[tex].image.m_image.m_handle, dymmyTextureMemoryBarriers[tex], VK_PIPELINE_STAGE_2_COPY_BIT,
                VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        }

		BlitzenVulkan::BeginCommandBuffer(pRenderer->m_commandsContext[0].m_transferCmdB, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        BlitzenVulkan::PipelineBarrier(pRenderer->m_commandsContext[0].m_transferCmdB, 0, nullptr, 0, nullptr, BlitzenCore::Ce_MaxTextureCount - dummyTextureOffset, 
            &dymmyTextureMemoryBarriers[dummyTextureOffset]);

        BlitzenVulkan::SubmitCommandBuffer(pRenderer->m_transferQueue.handle, pRenderer->m_commandsContext[0].m_transferCmdB, 0, nullptr, 0, nullptr, VK_NULL_HANDLE);
        vkQueueWaitIdle(pRenderer->m_transferQueue.handle);
	}
}