#include "Renderer/BlitzenVulkan/Context/vulkanRenderer.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vulkanCommands.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vkRuntimeHelpers.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanPipelines.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanRNDResources.h"
#include "Core/Events/blitTimeManager.h"
#include "BlitzenMathLibrary/blitML.h"

// Not necessary since I have my own math library
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"

namespace BlitzenVulkan
{
    void VulkanRenderer::DrawWhileWaiting(float deltaTime)
    {
        auto& cmd = m_commandsContext[0];
        auto colorAttachmentWorkingLayout = VK_IMAGE_LAYOUT_GENERAL;

        vkWaitForFences(m_device, 1, &cmd.m_uiFence.handle, VK_TRUE, ce_fenceTimeout);
        VK_CHECK_MSG(vkResetFences(m_device, 1, &cmd.m_uiFence.handle));

        vkAcquireNextImageKHR(m_device, m_swapchain.m_handle, ce_swapchainImageTimeout, cmd.m_swapchainSemaphore.handle, VK_NULL_HANDLE, &m_swapchainIDX);
        auto swapchainImage{ m_swapchain.m_images[m_swapchainIDX] };
        auto swapchainImageView{ m_swapchain.m_views[m_swapchainIDX] };

        // The command buffer recording begin here (stops when submit is called)
        BeginCommandBuffer(cmd.m_uiGraphicsCmdBuffer, 0);

        // The viewport and scissor are dynamic, so they should be set here
        DefineViewportAndScissor(cmd.m_uiGraphicsCmdBuffer, m_swapchain.m_extent);

        // Attachment barriers for layout transitions before rendering
        VkImageMemoryBarrier2 colorAttachmentDefinitionBarrier{};
        ImageMemoryBarrier(swapchainImage, colorAttachmentDefinitionBarrier, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 
            colorAttachmentWorkingLayout, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        PipelineBarrier(cmd.m_uiGraphicsCmdBuffer, 0, nullptr, 0, nullptr, 1, &colorAttachmentDefinitionBarrier);

        VkRenderingAttachmentInfo colorAttachmentInfo{};
        CreateRenderingAttachmentInfo(colorAttachmentInfo, swapchainImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE, { 0.1f, 0.2f, 0.3f, 0 });
        BeginRendering(cmd.m_uiGraphicsCmdBuffer, m_swapchain.m_extent, { 0, 0 }, 1, &colorAttachmentInfo, nullptr, nullptr);

        vkCmdBindPipeline(cmd.m_uiGraphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.m_trianglePso.handle);

        m_pipelines.m_loadingTriangleVertexColor *= cos(deltaTime);
        vkCmdPushConstants(cmd.m_uiGraphicsCmdBuffer, m_pipelines.m_triangleLayout.handle, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BlitML::vec3), &m_pipelines.m_loadingTriangleVertexColor);

        vkCmdDraw(cmd.m_uiGraphicsCmdBuffer, 3, 1, 0, 0);

        vkCmdEndRendering(cmd.m_uiGraphicsCmdBuffer);

        // Create a barrier for the swapchain image to transition to present optimal
        VkImageMemoryBarrier2 presentImageBarrier{};
        ImageMemoryBarrier(swapchainImage, presentImageBarrier,VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);
        // exectute
        PipelineBarrier(cmd.m_uiGraphicsCmdBuffer, 0, nullptr, 0, nullptr, 1, &presentImageBarrier);

        VkSemaphoreSubmitInfo waitSemaphores{};
        CreateSemahoreSubmitInfo(waitSemaphores, cmd.m_swapchainSemaphore.handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
        VkSemaphoreSubmitInfo signalSemaphore{};
        CreateSemahoreSubmitInfo(signalSemaphore, cmd.m_renderSemaphore.handle, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

        SubmitCommandBuffer(m_graphicsQueue.handle, cmd.m_uiGraphicsCmdBuffer, 1, &waitSemaphores, 1, &signalSemaphore, cmd.m_uiFence.handle);
    }
}