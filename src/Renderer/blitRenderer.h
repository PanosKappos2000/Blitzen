#pragma once

#include "BlitzenVulkan/vulkanRenderer.h"
#include "Renderer/blitRenderingResources.h"
#include "Game/blitCamera.h"

namespace BlitzenEngine
{
    enum class ActiveRenderer : uint8_t
    {
        Vulkan = 0,
        Directx12 = 1,

        MaxRenderers = 2
    };

    // Takes a smart pointer to a vulkan renderer and initalizes it. Succesfully returns if it gets initialized
    uint8_t CreateVulkanRenderer(BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>& pVulkan, 
    uint32_t windowWidth, uint32_t windowHeight);

    // Small fuction that tells the engine if Vulkan is active
    uint8_t isVulkanInitialized();

    // This function checks if the requested active renderer is available
    uint8_t CheckActiveRenderer(ActiveRenderer ar);

    // The parameters for this functions will be tidied up later
    void SetupRequestedRenderersForDrawing(RenderingResources* pResources, size_t drawCount);

    struct RuntimeDebugValues
    {
        uint8_t isDebugPyramidActive = 0;
        uint8_t m_occlusionCulling = 1;
        uint8_t m_lodEnabled = 1; 
    };

    void DrawFrame(Camera& camera, Camera* pMovingCamera, size_t drawCount, 
    uint32_t windowWidth, uint32_t windowHeight, uint8_t windowResize, 
    ActiveRenderer ar, RuntimeDebugValues* pDebugValues = nullptr);

    void ShutdownRenderers();
}