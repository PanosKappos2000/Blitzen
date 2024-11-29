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

        VkSwapchainKHR swapchain;
        VkExtent2D swapchainExtent;
        VkFormat swapchainFormat;
        BlitCL::DynamicArray<VkImage> swapchainImages;
        BlitCL::DynamicArray<VkImageView> swapchainImageViews;
    };

    struct Queue
    {
        uint32_t index;
        VkQueue handle;
        uint8_t hasIndex = 0;
    };

    // This struct holds any vulkan structure (buffers, sync structures etc), that need to have an instance for each frame in flight
    struct FrameTools
    {
        VkCommandPool mainCommandPool;
        VkCommandBuffer commandBuffer;

        VkFence inFlightFence;
        VkSemaphore imageAcquiredSemaphore;
        VkSemaphore readyToPresentSemaphore;
    };

    class VulkanRenderer
    {
    public:
        void Init(void* pPlatformState, uint32_t windowWidth, uint32_t windowHeight);

        /* ---------------------------------------------------------------------------------------------------------
            2nd part of Vulkan initialization. Gives scene data in arrays and vulkan uploads the data to the GPU
        ------------------------------------------------------------------------------------------------------------ */
        void UploadDataToGPUAndSetupForRendering();

        /*-----------------------------------------------------------------------------------------------
            Renders the world each frame. 
            If blitzen ever supports other graphics APIs, 
            they will have the same drawFrame function and will define their own render data structs
        -------------------------------------------------------------------------------------------------*/
        void DrawFrame(void* pRenderData);

        // Kills the renderer and cleans up allocated handles and resources
        void Shutdown();

    private:

        void FrameToolsInit();

    private:

        VkDevice m_device;

        VmaAllocator m_allocator;

        VkAllocationCallbacks* m_pCustomAllocator = nullptr;

        InitializationHandles m_initHandles;

        Queue m_graphicsQueue;
        Queue m_presentQueue;
        Queue m_computeQueue;

        VkCommandPool m_placeholderCommandPool;
        VkCommandBuffer m_placeholderCommands;

        AllocatedImage m_colorAttachment;
        AllocatedImage m_depthAttachment;
        VkExtent2D m_drawExtent;

        // This holds tools that need to be unique for each frame in flight
        FrameTools m_frameToolsList[BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT];

        size_t m_currentFrame = 0;

        // Holds stats that give information about how the vulkanRenderer is operating
        VulkanStats m_stats;
    };




    /* ----------------------
        Vulkan Resources 
    ------------------------- */

    void CreateBuffer(VmaAllocator allocator, AllocatedBuffer& buffer, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, VkDeviceSize bufferSize, 
    VmaAllocationCreateFlags allocationFlags);

    void CreateImage(VkDevice device, VmaAllocator allocator, AllocatedImage& image, VkExtent3D extent, VkFormat format, VkImageUsageFlags, 
    uint8_t loadMipmaps = 0);

    void CopyImageToImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout, 
    VkExtent2D srcImageSize, VkExtent2D dstImageSize, VkImageSubresourceLayers& srcImageSL, VkImageSubresourceLayers& dstImageSL);




    /*--------------------------------------
        Command Buffer helper functions
    ---------------------------------------*/

    void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags);

    void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer commandBuffer, VkSemaphore waitSemaphore, VkPipelineStageFlags2 waitPipelineStage,
    VkSemaphore signalSemaphore, VkPipelineStageFlags2 signalPipelineStage, VkFence fence = VK_NULL_HANDLE);


    /*--------------------------------------------------------------
        Pipeline barriers (implemented in vulkanResources.cpp)
    --------------------------------------------------------------*/

    void PipelineBarrier(VkCommandBuffer commandBuffer, uint32_t memoryBarrierCount, VkMemoryBarrier2* pMemoryBarriers, uint32_t bufferBarrierCount, 
    VkBufferMemoryBarrier2* pBufferBarriers, uint32_t imageBarrierCount, VkImageMemoryBarrier2* pImageBarriers);

    void ImageMemoryBarrier(VkImage image, VkImageMemoryBarrier2& barrier, VkPipelineStageFlags2 firstSyncStage, VkAccessFlags2 firstAccessStage, 
    VkPipelineStageFlags2 secondSyncStage, VkAccessFlags2 secondAccessStage, VkImageLayout oldLayout, VkImageLayout newLayout, 
    VkImageSubresourceRange& imageSR);
}