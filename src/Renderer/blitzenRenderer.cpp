#include "blitRenderer.h"
#include "Engine/blitzenEngine.h"

#define GET_RENDERER() RenderingSystem::GetRenderingSystem();

namespace BlitzenEngine
{
    RenderingSystem* RenderingSystem::s_pRenderer = nullptr;

    RenderingSystem::RenderingSystem()
    {
        s_pRenderer = this;

        // Opengl is only available on windows
        #ifndef _WIN32
            #undef BLITZEN_OPENGL
            #undef BLITZEN_DX12
        #endif

        // Automatically sets dx12 as the active api when on windows
        #ifdef _WIN32
            #define BLIT_DX12_ACTIVE_GRAPHICS_API
        #endif

        #ifdef BLITZEN_VULKAN
            bVk = vulkan.Init(BlitzenEngine::ce_initialWindowWidth, BlitzenEngine::ce_initialWindowHeight);
        #endif

        #ifdef BLITZEN_OPENGL
            bGl = opengl.Init(BlitzenEngine::ce_initialWindowWidth, BlitzenEngine::ce_initialWindowHeight);
        #endif

        // Automatically starts with vulkan, I will change this later
        #ifdef BLIT_VK_ACTIVE_GRAPHICS_API
        activeRenderer = ActiveRenderer::Vulkan;
        #elif BLIT_GL_ACTIVE_GRAPHICS_API
        activeRenderer = ActiveRenderer::Opengl;
        #endif

        if (!CheckActiveAPI())
        {
            #ifdef BLITZEN_VULKAN
                bVk = 0;
            #endif

            #ifdef BLITZEN_OPENGL
                bGl = 0;
            #endif

            BLIT_ERROR("No graphics API available")
        }
    }

    RenderingSystem::~RenderingSystem()
    {
        s_pRenderer = nullptr;
    }

    uint8_t RenderingSystem::CheckActiveAPI()
    {
        switch(activeRenderer)
        {
            case ActiveRenderer::Vulkan:
                return bVk; 
            case ActiveRenderer::Opengl:
                return bGl;     
            case ActiveRenderer::Direct3D12:
                return bDx12;
            default:
                return 0;
        }
    }

    uint8_t RenderingSystem::SetActiveAPI(ActiveRenderer ar)
    {
        if(CheckActiveAPI())
        {
            ClearCurrentActiveRenderer();
            /*if(ar == ActiveRenderer::Vulkan)
                gpVulkan->SetupForSwitch();This is supposed to fix a switching to Vulkan bug, but I got bored while writing it*/
            activeRenderer = ar;
            return 1;
        }

        return 0;
    }

    void RenderingSystem::ClearCurrentActiveRenderer()
    {
        switch(RenderingSystem::GetRenderingSystem()->activeRenderer)
        {
            case ActiveRenderer::Vulkan:
            {
                /*if (bVk)
                {
                    vulkan.ClearFrame();
                }*/
                break;
            }
            default:
            {
                break;
            }
        }
    }

    uint8_t RenderingSystem::SetupRequestedRenderersForDrawing(RenderingResources* pResources, uint32_t drawCount, Camera& camera)
    {
        if(drawCount <= 0)
        {
            BLIT_ERROR("Nothing to draw, Renderer not setup")
            return 0;
        }

        if(drawCount > ce_maxRenderObjects)
        {
            BLIT_ERROR("The render object count %i is higher than the current maximum allowed by the engine: %i", drawCount, ce_maxRenderObjects)
            return 0;
        }

        uint8_t isThereRendererOnStandby = 0;

        #ifdef BLITZEN_VULKAN
            if(bVk)
            {
                if(!vulkan.SetupForRendering(pResources, camera.viewData.pyramidWidth, camera.viewData.pyramidHeight))
                {
                    BLIT_ERROR("Could not initialize Vulkan. If this is the active graphics API, it needs to be swapped")
                    bVk = 0;
                }
                else
                    isThereRendererOnStandby = 1;
            }
        #endif

        #ifdef BLITZEN_OPENGL
            if(bGl)
            {
                if(!opengl.SetupForRendering(pResources))
                {
                    BLIT_ERROR("Could not initialize OPENGL. If this is the active graphics API, it needs to be swapped")
                    bGl = 0;
                }
                else
                    isThereRendererOnStandby = 1;
            }
        #endif

        return isThereRendererOnStandby;
    }

    void RenderingSystem::DrawFrame(Camera& camera, uint32_t drawCount)
    {
        // Check that the pointer for the active renderer is not Null
        if(!CheckActiveAPI())
        {
            // I could throw a warning here but it would fill the screen with the same error message over and over
            return;
        }

        // Call draw frame for the active renderer
        switch(activeRenderer)
        {
            case ActiveRenderer::Vulkan:
            {
                #ifdef BLITZEN_VULKAN
                BlitzenVulkan::DrawContext vkContext{ &camera, drawCount, occlusionCullingOn, lodEnabled };
                // Let Vulkan do its thing
                vulkan.DrawFrame(vkContext);
                #endif

                break;
            }
            case ActiveRenderer::Opengl:
            {
                #ifdef BLITZEN_OPENGL
                BlitzenGL::DrawContext glContext{&camera, drawCount, occlusionCullingOn, lodEnabled};
                opengl.DrawFrame(glContext);
                #endif
                break;
            }
            default:
                break;
        }
    }

    void RenderingSystem::ShutdownRenderers()
    {
        #ifdef BLITZEN_VULKAN
        if(bVk)
            vulkan.Shutdown();
        #endif

        #ifdef BLITZEN_OPENGL
        if(bGl)
            opengl.Shutdown();
        #endif
    }
}