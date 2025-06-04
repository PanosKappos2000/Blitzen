#pragma once
#include "vulkanContext.h"
#include "Renderer/Resources/Textures/blitTextures.h"
#include "Renderer/Interface/blitRendererInterface.h"

namespace BlitzenVulkan
{

    class VulkanRenderer
    {

    public:

        VulkanRenderer() = default;
        VulkanRenderer operator = (const VulkanRenderer& vk) = delete;
        VulkanRenderer(const VulkanRenderer& vk) = delete;

        ~VulkanRenderer();
        

        // Initalizes the Vulkan API.
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight, void* pPlatformHandle);

        // Sets up the Vulkan renderer for drawing according to the resources loaded by the engine
        uint8_t SetupForRendering(BlitzenEngine::DrawContext& drawContext);

        // Needed for dx12, not used here for now
        void FinalSetup();

        // Function for DDS texture loading
        uint8_t UploadTexture(const char* filepath);

        // Shows a loading screen while waiting for resources to be loaded
        void DrawWhileWaiting(float deltaTime);

        void Update(const BlitzenEngine::DrawContext& context);

        // Called each frame to draw the scene that is requested by the engine
        void DrawFrame(BlitzenEngine::DrawContext& context);

        // When a dynamic object moves, it should call this function to update the staging buffer
        void UpdateObjectTransform(uint32_t transformId, BlitzenEngine::MeshTransform* pTransform);

        inline VulkanStats GetStats() const { return m_stats; }

    public:

        // This struct holds any vulkan structure (buffers, sync structures etc), that need to have an instance for each frame in flight
        struct FrameTools
        {
            CommandPool mainCommandPool;
            VkCommandBuffer commandBuffer;

            CommandPool transferCommandPool;
            VkCommandBuffer transferCommandBuffer;

            CommandPool computeCommandPool;
            VkCommandBuffer computeCommandBuffer;

            SyncFence preCulsterCullingFence;
            SyncFence inFlightFence;

            Semaphore imageAcquiredSemaphore;
            Semaphore buffersReadySemaphore;
            Semaphore readyToPresentSemaphore;

            Semaphore preClusterCullingDoneSemaphore;

            uint8_t Init(VkDevice device, Queue graphicsQueue, Queue transferQueue, Queue computeQueue);
        };

    public:

        MemoryCrucialHandles m_memoryCrucials;

        // Vulkan API and memory crucials
        VkInstance m_instance;
        VkPhysicalDevice m_physicalDevice;
        VkDevice m_device;

        SurfaceKHR m_surface;
        Swapchain m_swapchainValues;

        VmaAllocator m_allocator;

        VulkanStats m_stats;

        Queue m_graphicsQueue;
        Queue m_presentQueue;
        Queue m_computeQueue;
        Queue m_transferQueue;

    private:

        VkDebugUtilsMessengerEXT m_debugMessenger;

        uint32_t m_drawWidth;
        uint32_t m_drawHeight;

        ROResources m_readOnlies;

        RWResources m_readWrites[ce_framesInFlight];

        DescriptorContext m_descriptorContext;

        PipelineContext m_pipelines;

        FrameTools m_frameToolsList[ce_framesInFlight];

        // Used for any loading pipeline
        CommandPool m_idleCommandBufferPool;
        VkCommandBuffer m_idleDrawCommandBuffer;

        // Frame tools index
        uint8_t m_currentFrame;
        
    };


    // Creates the swapchain
    uint8_t CreateSwapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, uint32_t windowWidth, uint32_t windowHeight, 
        Queue graphicsQueue, Queue presentQueue, Queue computeQueue, VkAllocationCallbacks* pCustomAllocator, Swapchain& newSwapchain, VkSwapchainKHR oldSwapchain);

    uint8_t BuildBlas(VkInstance instance, VkDevice device, VmaAllocator vma, VulkanRenderer::FrameTools& frameTools, VkQueue queue, BlitzenEngine::DrawContext& context, 
        ROResources& readOnlies);

    uint8_t BuildTlas(VkInstance instance, VkDevice device, VmaAllocator vma, VulkanRenderer::FrameTools& frameTools, VkQueue queue, ROResources& readOnlies, 
        BlitzenEngine::DrawContext& context);
}