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
            CreateVulkanRenderer(vulkan, BLITZEN_WINDOW_WIDTH, BLITZEN_WINDOW_HEIGHT);
        #endif

        #ifdef BLITZEN_OPENGL
            #if _MSC_VER
            CreateOpenglRenderer(opengl, BLITZEN_WINDOW_WIDTH, BLITZEN_WINDOW_HEIGHT);
            #endif
        #endif

        if(pVulkan)
            activeRenderer = ActiveRenderer::Vulkan;
        else if(pGl)
            activeRenderer = ActiveRenderer::Opengl;
        else
            activeRenderer = ActiveRenderer::MaxRenderers;

        BLIT_ASSERT(CheckActiveRenderer(activeRenderer))
    }

    RenderingSystem::~RenderingSystem()
    {
        s_pRenderer = nullptr;
    }

    uint8_t CreateVulkanRenderer(BlitzenVulkan::VulkanRenderer& vulkan, 
    uint32_t windowWidth, uint32_t windowHeight)
    {
        
        // Call the init function and store the result in the systems boolean for Vulkan
        if(vulkan.Init(windowWidth, windowHeight))
        {
            RenderingSystem::GetRenderingSystem()->pVulkan = &vulkan;
            return 1;
        }   
        return 0;
    }

    uint8_t CreateOpenglRenderer(BlitzenGL::OpenglRenderer& renderer, uint32_t windowWidth, uint32_t windowHeight)
    {
        #ifdef linux
            BLIT_ERROR("Opengl not available for Linux builds at the moment")
            return 0;
        #else
            if(renderer.Init(windowWidth, windowHeight))
            {
                RenderingSystem::GetRenderingSystem()->pGl = &renderer;
                return 1;
            }
            else
            {
                return 0;
            }
        #endif
    }

    uint8_t isVulkanInitialized() { return RenderingSystem::GetRenderingSystem()->pVulkan != nullptr; }

    uint8_t IsOpenglInitialized() {return RenderingSystem::GetRenderingSystem()->pGl != nullptr;}

    uint8_t CheckActiveRenderer(ActiveRenderer ar)
    {
        switch(ar)
        {
            case ActiveRenderer::Vulkan:
                return isVulkanInitialized();
            case ActiveRenderer::Opengl:
                return IsOpenglInitialized();
            case ActiveRenderer::Directx12:
                return 0;
            default:
                return 0;
        }
    }

    uint8_t SetActiveRenderer(ActiveRenderer ar)
    {
        if(CheckActiveRenderer(ar))
        {
            ClearCurrentActiveRenderer();
            /*if(ar == ActiveRenderer::Vulkan)
                gpVulkan->SetupForSwitch();This is supposed to fix a switching to Vulkan bug, but I got bored while writing it*/
            RenderingSystem::GetRenderingSystem()->activeRenderer = ar;
            return 1;
        }

        return 0;
    }

    void ClearCurrentActiveRenderer()
    {
        switch(RenderingSystem::GetRenderingSystem()->activeRenderer)
        {
            case ActiveRenderer::Vulkan:
            {
                BlitzenVulkan::VulkanRenderer* pVulkan = RenderingSystem::GetRenderingSystem()->pVulkan;
                if(pVulkan)
                {
                    pVulkan->ClearFrame();
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    uint8_t SetupRequestedRenderersForDrawing(RenderingResources* pResources, size_t drawCount, Camera& camera)
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

        /*
            Setting up data that will be needed by renderers at draw time for culling and other operations
        */
        // Pass camera values here so that they are available before the first frame
        pResources->shaderData.projectionView = camera.projectionViewMatrix;
        pResources->shaderData.viewPosition = camera.position;
        pResources->shaderData.view = camera.viewMatrix;

        // Frustum planes
        BlitML::vec4 frustumX = BlitML::NormalizePlane(camera.projectionTranspose.GetRow(3) + camera.projectionTranspose.GetRow(0)); // x + w < 0
        BlitML::vec4 frustumY = BlitML::NormalizePlane(camera.projectionTranspose.GetRow(3) + camera.projectionTranspose.GetRow(1)); // y+ w < 0;
        pResources->cullingData.frustumRight = frustumX.x;
        pResources->cullingData.frustumLeft = frustumX.z;
        pResources->cullingData.frustumTop = frustumY.y;
        pResources->cullingData.frustumBottom = frustumY.z;

        // Culling data for occlusion culling
        pResources->cullingData.proj0 = camera.projectionMatrix[0];
        pResources->cullingData.proj5 = camera.projectionMatrix[5];

        // Lod target. Used in the threshold calculation formula along with the target's scale and its distance from the camera
        pResources->cullingData.lodTarget = (2 / pResources->cullingData.proj5) * (1.f / float(camera.windowHeight));

        uint8_t isThereRendererOnStandby = 0;

        BlitzenVulkan::VulkanRenderer* pVulkan = RenderingSystem::GetRenderingSystem()->pVulkan;
        if(pVulkan)
        {
            if(!pVulkan->SetupForRendering(pResources))
            {
                BLIT_ERROR("Could not initialize Vulkan. If this is the active graphics API, it needs to be swapped")
                pVulkan = nullptr;
                isThereRendererOnStandby = isThereRendererOnStandby || 0;
            }
            else
                isThereRendererOnStandby = 1;
        }

        #if _MSC_VER
        BlitzenGL::OpenglRenderer* pGL = RenderingSystem::GetRenderingSystem()->pGl;
        if(pGL)
        {
            if(!pGL->SetupForRendering(pResources))
            {
                BLIT_ERROR("Could not initialize OPENGL. If this is the active graphics API, it needs to be swapped")
                pGL = nullptr;
                isThereRendererOnStandby = isThereRendererOnStandby || 0;
            }
            else
                isThereRendererOnStandby = 1;
        }
        #endif

        return isThereRendererOnStandby;
    }

    void DrawFrame(Camera& camera, Camera* pMovingCamera, size_t drawCount, 
    uint8_t windowResize, RenderContext& context)
    {
        RenderingSystem* pSystem = GET_RENDERER()
        // Check that the pointer for the active renderer is not Null
        if(!CheckActiveRenderer(pSystem->activeRenderer))
        {
            // I could throw a warning here but it would fill the screen with the same error message over and over
            return;
        }

        GlobalShaderData& shaderData = context.globalShaderData;

        // Pass the result of projection * view from the detatched camera if it moved since last frame
        // In case the camera is indeed detatched from the main, the camera will move, but culling will not change for debugging purposes
        if(pMovingCamera->cameraDirty)
            shaderData.projectionView = pMovingCamera->projectionViewMatrix;

        // Pass the camera position and view matrix from the main camera, it if has moved since last frame
        if(camera.cameraDirty)
        {
            shaderData.viewPosition = camera.position;
            shaderData.view = camera.viewMatrix;
        }

        // Global lighting parameters will be written to the global shader data buffer
        // TODO: Add some logic to see if these value change (currently they never change and are unused)
        /*shaderData.sunlightDir = BlitML::vec3 sunDir(-0.57735f, -0.57735f, 0.57735f);
        shaderData.sunlightColor = BlitML::vec4 sunColor(0.8f, 0.8f, 0.8f, 1.0f);*/

        CullingData& cullingData = context.cullingData;

        // Create the frustum planes based on the current projection matrix, will be written to the culling data buffer
        if(windowResize)
        {
            BlitML::vec4 frustumX = BlitML::NormalizePlane(camera.projectionTranspose.GetRow(3) + camera.projectionTranspose.GetRow(0)); // x + w < 0
            BlitML::vec4 frustumY = BlitML::NormalizePlane(camera.projectionTranspose.GetRow(3) + camera.projectionTranspose.GetRow(1)); // y+ w < 0;
            cullingData.frustumRight = frustumX.x;
            cullingData.frustumLeft = frustumX.z;
            cullingData.frustumTop = frustumY.y;
            cullingData.frustumBottom = frustumY.z;

            // Culling data for occlusion culling
            cullingData.proj0 = camera.projectionMatrix[0];
            cullingData.proj5 = camera.projectionMatrix[5];

            // Update the lod target as well since it uses window dimensions
            cullingData.lodTarget = (2 / cullingData.proj5) * (1.f / float(camera.windowWidth));
        }

        // The near and far planes of the frustum will use the camera directly
        cullingData.zNear = camera.zNear;
        cullingData.zFar = camera.drawDistance;

        cullingData.drawCount = static_cast<uint32_t>(drawCount);

        context.windowResize = windowResize;
        context.windowWidth = static_cast<uint32_t>(camera.windowWidth);
        context.windowHeight = static_cast<uint32_t>(camera.windowHeight);

        cullingData.occlusionEnabled = pSystem->occlusionCullingOn;
        cullingData.lodEnabled = pSystem->lodEnabled;

        // Call draw frame for the active renderer
        switch(pSystem->activeRenderer)
        {
            case ActiveRenderer::Vulkan:
            {
                // Let Vulkan do its thing
                pSystem->pVulkan->DrawFrame(context);

                break;
            }
            case ActiveRenderer::Opengl:
            {
                #if _MSC_VER
                    pSystem->pGl->DrawFrame(context);
                #endif
                break;
            }
            default:
                break;
        }
    }

    void ShutdownRenderers()
    {
        RenderingSystem* pSystem = GET_RENDERER()
        if(pSystem->pVulkan)
            pSystem->pVulkan->Shutdown();

        #if _MSC_VER
        if(pSystem->pGl)
            pSystem->pGl->Shutdown();
        #endif
    }
}