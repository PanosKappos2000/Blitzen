#include "blitRenderer.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenEngine
{
    inline BlitzenVulkan::VulkanRenderer* gpVulkan = nullptr;
    // For now, the memory crucial handles will be defined here globally. The destructor will be called automatically after the vk renderer is destroyed
    inline BlitzenVulkan::MemoryCrucialHandles vulkanCrucials;

    inline BlitzenGL::OpenglRenderer* gpGl = nullptr;

    inline void* gpDx12 = nullptr;

    inline ActiveRenderer iActive = ActiveRenderer::MaxRenderers;

    uint8_t CreateVulkanRenderer(BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>& pVulkan, 
    uint32_t windowWidth, uint32_t windowHeight)
    {
        if(pVulkan.Data())
        {
            // Call the init function and store the result in the systems boolean for Vulkan
            if(pVulkan.Data()->Init(windowWidth, windowHeight))
            {
                gpVulkan = pVulkan.Data();
                return 1;
            }
            
            return 0;
        }
        else
        {
            return 0;
        }
    }

    uint8_t CreateOpenglRenderer(BlitzenGL::OpenglRenderer& renderer, uint32_t windowWidth, uint32_t windowHeight)
    {
        #if LINUX
            BLIT_ERROR("Opengl not available for Linux builds at the moment")
            return 0;
        #else
            if(renderer.Init(windowWidth, windowHeight))
            {
                gpGl = &renderer;
                return 1;
            }
            else
            {
                return 0;
            }
        #endif
    }

    uint8_t isVulkanInitialized() { return gpVulkan != nullptr; }

    uint8_t IsOpenglInitialized() {return gpGl != nullptr;}

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
            iActive = ar;
            return 1;
        }

        return 0;
    }

    void ClearCurrentActiveRenderer()
    {
        switch(iActive)
        {
            case ActiveRenderer::Vulkan:
            {
                if(gpVulkan)
                {
                    gpVulkan->ClearFrame();
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

        uint8_t isThereRendererOnStandby = 0;

        if(gpVulkan)
        {
            if(!gpVulkan->SetupForRendering(pResources))
            {
                BLIT_ERROR("Could not initialize Vulkan. If this is the active graphics API, it needs to be swapped")
                gpVulkan = nullptr;
                isThereRendererOnStandby = isThereRendererOnStandby || 0;
            }

            isThereRendererOnStandby = 1;
        }

        #if _MSC_VER
        if(gpGl)
        {
            if(!gpGl->SetupForRendering(pResources))
            {
                BLIT_ERROR("Could not initialize OPENGL. If this is the active graphics API, it needs to be swapped")
                gpGl = nullptr;
                isThereRendererOnStandby = isThereRendererOnStandby || 0;
            }

            isThereRendererOnStandby = 1;
        }
        #endif

        return isThereRendererOnStandby;
    }

    void DrawFrame(Camera& camera, Camera* pMovingCamera, size_t drawCount, 
    uint32_t windowWidth, uint32_t windowHeight, uint8_t windowResize, 
    ActiveRenderer ar, RenderContext& context, RuntimeDebugValues* pDebugValues /*= nullptr*/)
    {
        // Check that the pointer for the active renderer is not Null
        if(!CheckActiveRenderer(ar))
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
        }

        // The near and far planes of the frustum will use the camera directly
        cullingData.zNear = camera.zNear;
        cullingData.zFar = camera.drawDistance;

        cullingData.drawCount = static_cast<uint32_t>(drawCount);


        context.windowResize = windowResize;
        context.windowWidth = windowWidth;
        context.windowHeight = windowHeight;

        // Debug values, controlled by inputs
        if(pDebugValues)
        {
            //cullingData.debugPyramid = pDebugValues->isDebugPyramidActive; // f2 to change (inoperable)
            cullingData.occlusionEnabled = pDebugValues->m_occlusionCulling; // f3 to change
            cullingData.lodEnabled = pDebugValues->m_lodEnabled;// f4 to change
        }

        switch(iActive)
        {
            case ActiveRenderer::Vulkan:
            {
                // Let Vulkan do its thing
                gpVulkan->DrawFrame(context);

                break;
            }
            case ActiveRenderer::Opengl:
            {
                #if _MSC_VER
                    gpGl->DrawFrame(context);
                #endif
                break;
            }
            default:
                break;
        }
    }

    void ShutdownRenderers()
    {
        if(gpVulkan)
            gpVulkan->Shutdown(vulkanCrucials);

        if(gpGl)
            gpGl->Shutdown();
    }
}