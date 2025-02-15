#include "blitRenderer.h"
#include "Engine/blitzenEngine.h"

#define GET_RENDERER() RenderingSystem::GetRenderingSystem();

namespace BlitzenEngine
{
    RenderingSystem* RenderingSystem::s_pRenderer = nullptr;

    RenderingSystem::RenderingSystem()
    {
        s_pRenderer = this;

        #ifdef BLITZEN_VULKAN
            bVk = vulkan.Init(BLITZEN_WINDOW_WIDTH, BLITZEN_WINDOW_HEIGHT);
        #endif

        #ifdef BLITZEN_OPENGL
            #if _MSC_VER
                bGl = opengl.Init(BLITZEN_WINDOW_WIDTH, BLITZEN_WINDOW_HEIGHT);
            #endif
        #endif

        // Automatically starts with vulkan, I will change this later
        #ifdef BLIT_VK_ACTIVE_GRAPHICS_API
        activeRenderer = ActiveRenderer::Vulkan;
        #elif BLIT_GL_ACTIVE_GRAHPICS_API
        activeRenderer = ActiveRenderer::Opengl;
        #endif

        BLIT_ASSERT(CheckActiveAPI())
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
            case ActiveRenderer::Directx12:
                return 0;
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
                if(bVk)
                {
                    vulkan.ClearFrame();
                }
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

        if(drawCount > BLITZEN_MAX_DRAW_OBJECTS)
        {
            BLIT_ERROR("The render object count %i is higher than the current maximum allowed by the engine: %i", drawCount, BLITZEN_MAX_DRAW_OBJECTS)
            return 0;
        }

        uint8_t isThereRendererOnStandby = 0;

        if(bVk)
        {
            if(!vulkan.SetupForRendering(pResources, camera.viewData.pyramidWidth, camera.viewData.pyramidHeight))
            {
                BLIT_ERROR("Could not initialize Vulkan. If this is the active graphics API, it needs to be swapped")
                isThereRendererOnStandby = isThereRendererOnStandby || 0;
            }
            else
                isThereRendererOnStandby = 1;
        }

        #if _MSC_VER
        if(bGl)
        {
            if(!opengl.SetupForRendering(pResources))
            {
                BLIT_ERROR("Could not initialize OPENGL. If this is the active graphics API, it needs to be swapped")
                isThereRendererOnStandby = isThereRendererOnStandby || 0;
            }
            else
                isThereRendererOnStandby = 1;
        }
        #endif

        return isThereRendererOnStandby;
    }

    void RenderingSystem::DrawFrame(Camera& camera, Camera* pMovingCamera, uint32_t drawCount)
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
                BlitzenVulkan::DrawContext vkContext{ &camera, pMovingCamera, drawCount, occlusionCullingOn, lodEnabled };
                // Let Vulkan do its thing
                vulkan.DrawFrame(vkContext);

                break;
            }
            case ActiveRenderer::Opengl:
            {
                #if _MSC_VER
                    BlitzenGL::DrawContext glContext{&camera, pMovingCamera, drawCount, occlusionCullingOn, lodEnabled};
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
        RenderingSystem* pSystem = GET_RENDERER()
        vulkan.Shutdown();

        #if _MSC_VER
            opengl.Shutdown();
        #endif
    }
}