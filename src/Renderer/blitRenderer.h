#pragma once

#include "BlitzenVulkan/vulkanRenderer.h"
#include "BlitzenGl/openglRenderer.h"
#ifdef _WIN32
#include "BlitzenDX12/dx12Data.h"
#endif
#include "Renderer/blitRenderingResources.h"
#include "Game/blitCamera.h"

namespace BlitzenEngine
{
    enum class ActiveRenderer : uint8_t
    {
        Vulkan = 0,
        Direct3D12 = 1,
        Opengl = 2,

        MaxRenderers = 3
    };

    class RenderingSystem
    {
    public:

        RenderingSystem();

        ~RenderingSystem();

        inline static RenderingSystem* GetRenderingSystem() { return s_pRenderer; }

        inline uint8_t IsVulkanAvailable() { return bVk; }

        
        inline uint8_t IsOpenglAvailable() { return bGl; }

        void ShutdownRenderers();

    public:

        // This is here because the UploadDDSTexture function cannot be called by const reference
        #ifdef BLITZEN_VULKAN
        inline uint8_t GiveTextureToVulkan(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10,
        void* pData, const char* filepath) {
            return vulkan.UploadDDSTexture(header, header10, pData, filepath);
        }
        #else
        inline uint8_t GiveTextureToVulkan(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10,
            void* pData, const char* filepath) {
            BLIT_ERROR("Vulkan was never requested")
            return 0;
        }
        #endif

        // Same as the above
        #ifdef BLITZEN_OPENGL
        inline uint8_t GiveTextureToOpengl(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10,
        const char* filepath) { 
            return opengl.UploadTexture(header, header10, filepath);
        }
        #else
        inline uint8_t GiveTextureToOpengl(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10,
            const char* filepath) {
            BLIT_ERROR("Opengl was never requested")
            return 0;
        }
        #endif 

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
        #endif

        #ifdef BLITZEN_OPENGL
            BlitzenGL::OpenglRenderer opengl;
        #endif

        uint8_t bVk = 0;
        uint8_t bGl = 0;
        uint8_t bDx12 = 0;

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