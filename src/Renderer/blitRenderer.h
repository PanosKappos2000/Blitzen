#pragma once

#include "BlitzenVulkan/vulkanRenderer.h"
#include "BlitzenGl/openglRenderer.h"
#ifdef _WIN32
#include "BlitzenDX12/dx12Renderer.h"
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

    template<typename T>
    class RenderingSystem
    {
    public:

        RenderingSystem()
        {
            m_bBackendActive = m_backend.Init(BlitzenEngine::ce_initialWindowWidth, BlitzenEngine::ce_initialWindowHeight);
        }

        ~RenderingSystem()
        {
            if (m_bBackendActive)
                m_backend.Shutdown();
        }

        // The parameters for this functions will be tidied up later
        uint8_t SetupBackendRenderer(RenderingResources* pResources, uint32_t drawCount, Camera& camera)
        {
            if (!m_bBackendActive)
                return 0;

            if (drawCount <= 0)
            {
                BLIT_ERROR("Nothing to draw, Renderer not setup")
                    return 0;
            }

            if (drawCount > ce_maxRenderObjects)
            {
                BLIT_ERROR("The render object count %i is higher than the current maximum allowed by the engine: %i", drawCount, ce_maxRenderObjects)
                    return 0;
            }

            //T::SetupData vkData{}
            if (!m_backend.SetupForRendering(pResources, camera.viewData.pyramidWidth, camera.viewData.pyramidHeight))
                return 0;

            return 1;
        }

        void DrawFrame(Camera& camera, uint32_t drawCount)
        {
            if (!m_bBackendActive)
                return;

            DrawContext context{ &camera, drawCount, occlusionCullingOn, lodEnabled };
            // Let Vulkan do its thing
            m_backend.DrawFrame(context);
        }



    public:

        uint8_t UploadTexture(BlitzenEngine::DDS_HEADER& header, BlitzenEngine::DDS_HEADER_DXT10& header10,
        void* pData, const char* filepath) 
        {
            if(!m_bBackendActive)
                return 0;

            return m_backend.UploadTexture(header, header10, pData, filepath);
        }


    private:
        T m_backend;
        uint8_t m_bBackendActive = 0;
    
    public:

        // Debug values, these have no place here
        uint8_t freezeFrustum = 0;
        uint8_t debugPyramidActive = 0;
        uint8_t occlusionCullingOn = 1;
        uint8_t lodEnabled = 1;
    };


    
    // Switch the renderer debug values
    /*inline void ChangeFreezeFrustumState() {
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
    }*/
}