#pragma once

#include "BlitzenVulkan/vulkanRenderer.h"

#include "BlitzenGl/openglRenderer.h"

// The rendering resources are passed to the renderers to set up their global buffers
#include "Renderer/blitRenderingResources.h"

// The camera file is needed as it is passed on some functions for the renderers to access its values
#include "Game/blitCamera.h"

// Max draw calls allowed, if render objects go above this, the application will fail
#define BLITZEN_MAX_DRAW_OBJECTS    5'000'000

// Cluster rendering is not yet implemented, but I really wish I can try it out soon
#define BLITZEN_CLUSTER_RENDERING   0

namespace BlitzenEngine
{
    enum class ActiveRenderer : uint8_t
    {
        Vulkan = 0,
        Directx12 = 1,
        Opengl = 2,

        MaxRenderers = 3
    };

    // Takes a smart pointer to a vulkan renderer and initalizes it. Succesfully returns if it gets initialized
    uint8_t CreateVulkanRenderer(BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>& pVulkan, 
    uint32_t windowWidth, uint32_t windowHeight);

    // Small fuction that tells the engine if Vulkan is active
    uint8_t isVulkanInitialized();

    // Create the opengl renderer
    uint8_t CreateOpenglRenderer(BlitzenGL::OpenglRenderer& renderer, uint32_t windowWidth, uint32_t windowHeight);

    // Checks if opengl is initialized
    uint8_t IsOpenglInitialized();

    // When changing the renderer mid flight, the previous renderer might need to clear the frame
    void ClearCurrentActiveRenderer();
    // Sets the renderer / graphics API that will be used when drawing
    uint8_t SetActiveRenderer(ActiveRenderer ar);
    // This function checks if the requested active renderer is available
    uint8_t CheckActiveRenderer(ActiveRenderer ar);

    // The parameters for this functions will be tidied up later
    uint8_t SetupRequestedRenderersForDrawing(RenderingResources* pResources, size_t drawCount, Camera& camera);

    struct RuntimeDebugValues
    {
        uint8_t isDebugPyramidActive = 0;
        uint8_t m_occlusionCulling = 1;
        uint8_t m_lodEnabled = 1; 
    };

    void DrawFrame(Camera& camera, Camera* pMovingCamera, size_t drawCount, 
    uint32_t windowWidth, uint32_t windowHeight, uint8_t windowResize, 
    RenderContext& renderContext, uint8_t isDebugPyramidActive = 0,
    uint8_t m_occlusionCulling = 1, uint8_t m_lodEnabled = 1);

    void ShutdownRenderers();
}