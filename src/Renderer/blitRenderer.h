#pragma once

#include "BlitzenVulkan/vulkanRenderer.h"

#include "BlitzenGl/openglRenderer.h"

// The rendering resources are passed to the renderers to set up their global buffers
#include "Renderer/blitRenderingResources.h"

// The camera file is needed as it is passed on some functions for the renderers to access its values
#include "Game/blitCamera.h"

#ifndef __WIN32
    #undef BLITZEN_OPENGL
#endif

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

        #ifdef BLITZEN_VULKAN
        inline const BlitzenVulkan::VulkanRenderer& GetVulkan() { return vulkan; }
        #else
        inline const BlitzenVulkan::VulkanRenderer& GetVulkan() {
            BLIT_FATAL("Vulkan was never requested")
            BlitzenVulkan::VulkanRenderer dummy;
            return dummy;
        }
        #endif

        #ifdef BLITZEN_VULKAN
        inline uint8_t IsVulkanAvailable() { 
            return bVk; 
        }
        #else
        inline uint8_t IsVulkanAvailable() {
            BLIT_INFO("Vulkan not requested")
            return 0;
        }
        #endif

        #ifdef BLITZEN_OPENGL 
        inline const BlitzenGL::OpenglRenderer& GetOpengl() { return opengl; }
        #else
        inline const BlitzenGL::OpenglRenderer& GetOpengl() {
            BLIT_FATAL("Vulkan was never requested")
            BlitzenGL::OpenglRenderer dummy; 
            return dummy;
        }
        #endif

        #ifdef BLITZEN_OPENGL
        inline uint8_t IsOpenglAvailable() {
            return bGl;
        }
        #else
        inline uint8_t IsOpenglAvailable() {
            BLIT_INFO("Opengl not requested")
            return 0;
        }
        #endif

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