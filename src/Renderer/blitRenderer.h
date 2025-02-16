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

    class RenderingSystem
    {
    public:

        RenderingSystem();

        ~RenderingSystem();

        inline static RenderingSystem* GetRenderingSystem() { return s_pRenderer; }

        inline const BlitzenVulkan::VulkanRenderer& GetVulkan() { return vulkan; }
        inline uint8_t IsVulkanAvailable() { return bVk; }

        inline const BlitzenGL::OpenglRenderer& GetOpengl() { return opengl; }
        inline uint8_t IsOpenglAvailable() { return bGl; }

        void ShutdownRenderers();

    public:

        // This is here because the UploadDDSTexture function cannot be called by const reference
        inline uint8_t GiveTextureToVulkan(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10,
        void* pData, const char* filepath) { return vulkan.UploadDDSTexture(header, header10, pData, filepath); }

        // Same as the above
        inline uint8_t GiveTextureToOpengl(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10,
        const char* filepath) { return opengl.UploadTexture(header, header10, filepath); }

        // The parameters for this functions will be tidied up later
        uint8_t SetupRequestedRenderersForDrawing(RenderingResources* pResources, uint32_t drawCount, Camera& camera);

        void DrawFrame(Camera& camera, uint32_t drawCount);

        // Pointless feature that doesn't work
        uint8_t SetActiveAPI(ActiveRenderer newActiveAPI);
        void ClearCurrentActiveRenderer();

        void* pDx12 = nullptr;

    private:
        #ifdef BLITZEN_VULKAN
            BlitzenVulkan::VulkanRenderer vulkan;
            uint8_t bVk = 0;
        #endif

        #ifdef BLITZEN_OPENGL
            BlitzenGL::OpenglRenderer opengl;
            uint8_t bGl = 0;
        #endif

        ActiveRenderer activeRenderer = ActiveRenderer::MaxRenderers;
        uint8_t CheckActiveAPI();
    
    public:

        // Debug values 
        uint8_t freezeFrustum = 0;
        uint8_t debugPyramidActive = 0;
        uint8_t occlusionCullingOn = 1;
        uint8_t lodEnabled = 1;

    private:
        // Leaky singleton
        static RenderingSystem* s_pRenderer;
    };

    // Switch the renderer debug values
    inline void ChangeFreezeFrustumState() { 
        RenderingSystem::GetRenderingSystem()->freezeFrustum = !RenderingSystem::GetRenderingSystem()->freezeFrustum; 
    }
    inline void ChangeDebugPyramidActiveState() { 
        RenderingSystem::GetRenderingSystem()->debugPyramidActive = !RenderingSystem::GetRenderingSystem()->debugPyramidActive; 
    }
    inline void ChangeOcclusionCullingState() { 
        RenderingSystem::GetRenderingSystem()->occlusionCullingOn = !RenderingSystem::GetRenderingSystem()->occlusionCullingOn; 
    }
    inline void ChangeLodEnabledState()  { 
        RenderingSystem::GetRenderingSystem()->lodEnabled  = !RenderingSystem::GetRenderingSystem()->lodEnabled; 
    }
}