#pragma once

#include "vulkanData.h"

namespace BlitzenVulkan
{
    struct InitializationHandles
    {
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkPhysicalDevice chosenGpu;
    };

    // This struct holds any vulkan structure (buffers, sync structures etc), that need to have an instance for each frame in flight
    struct FrameTools
    {

    };

    class VulkanRenderer
    {
    public:
        void Init();

        /*-----------------------------------------------------------------------------------------------
            Renders the world each frame. 
            If blitzen ever supports other graphics APIs, 
            they will have the same drawFrame function and will define their own render data structs
        -------------------------------------------------------------------------------------------------*/
        void DrawFrame(void* pRenderData);

        // Kills the renderer and cleans up allocated handles and resources
        void Shutdown();

    private:

        VkDevice m_device;

        VkAllocationCallbacks* m_pCustomAllocator = nullptr;

        InitializationHandles m_initHandles;

        /*AllocatedImage m_colorAttachment;
        AllocatedImage m_depthAttachment;
        VkExtent2D m_drawExtent;*/

        FrameTools m_frameToolsList[BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT];

        size_t m_currentFrame = 0;
    };
}