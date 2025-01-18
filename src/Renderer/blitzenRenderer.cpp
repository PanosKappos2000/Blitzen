#include "blitRenderer.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenEngine
{
    inline uint8_t vulkanInitialized = 0;
    inline uint8_t dx12Initialized = 0;

    uint8_t CreateVulkanRenderer(BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>& pVulkan, 
    uint32_t windowWidth, uint32_t windowHeight)
    {
        if(pVulkan.Data())
        {
            // Call the init function and store the result in the systems boolean for Vulkan
            vulkanInitialized =  pVulkan.Data()->Init(windowWidth, windowHeight);
            return vulkanInitialized;
        }
        else
        {
            return 0;
        }
    }

    uint8_t isVulkanInitialized() { return vulkanInitialized; }

    uint8_t CheckActiveRenderer(ActiveRenderer ar)
    {
        switch(ar)
        {
            case ActiveRenderer::Vulkan:
                return vulkanInitialized;
            default:
                break;
        }
    }

    void SetupRequestedRenderersForDrawing(RenderingResources& resources, size_t drawCount, BlitzenVulkan::VulkanRenderer* pVulkan /* nullptr*/)
    {
        if(pVulkan && vulkanInitialized)
        {
            // The values that were loaded need to be passed to the vulkan renderere so that they can be loaded to GPU buffers
            BlitzenVulkan::GPUData vulkanData(resources.vertices, resources.indices, resources.meshlets, 
            resources.surfaces, resources.transforms, resources.meshletData);/* The contructor is needed for values 
            that are references instead of pointers */

            vulkanData.pTextures = resources.textures;
            vulkanData.textureCount = resources.currentTextureIndex;// Current texture index is equal to the size of the array of textures

            vulkanData.pMaterials = resources.materials;
            vulkanData.materialCount = resources.currentMaterialIndex;// Current material index is equal to the size of the material array

            vulkanData.pMeshes = resources.meshes;
            vulkanData.meshCount = resources.currentMeshIndex;// Current mesh index is equal to the size of the mesh array

            vulkanData.pGameObjects = resources.objects;
            vulkanData.gameObjectCount = resources.objectCount;

            // Draw count will be used to determine the size of draw and object buffers
            vulkanData.drawCount = drawCount;

            pVulkan->SetupForRendering(vulkanData);
        }
    }

    void DrawFrame(Camera& camera, Camera* pMovingCamera, size_t drawCount, 
    uint32_t windowWidth, uint32_t windowHeight, uint8_t windowResize, 
    void* pRendererBackend, ActiveRenderer ar, RuntimeDebugValues* pDebugValues /*= nullptr*/)
    {
        if(!pRendererBackend)
        {
            return;
        }

        switch(ar)
        {
            case ActiveRenderer::Vulkan:
            {
                BlitzenVulkan::VulkanRenderer* pVulkan = reinterpret_cast<BlitzenVulkan::VulkanRenderer*>(pRendererBackend);
                BlitzenVulkan::RenderContext renderContext;
                        
                // In the case that the window was resized, these are passed so that the swapchain can be recreated
                renderContext.windowResize = windowResize;
                renderContext.windowWidth = windowWidth;
                renderContext.windowHeight = windowHeight;

                // Pass the projection matrix to be used to promote objects to clip coordinates without promoting them to view coordinates
                renderContext.projectionMatrix = camera.projectionMatrix;

                // Pass the view matrix to be used to promote objects to view coordinates but not clip coordinates
                renderContext.viewMatrix = camera.viewMatrix;

                // The projection view is the result of projectionMatrix * viewMatrix
                // Calculated on the CPU to avoid doing it every vertex/mesh shader invocation
                // This one is changed even if the camera is detatched
                renderContext.projectionView = pMovingCamera->projectionViewMatrix;

                // Camera position is needed for some lighting calculations
                renderContext.viewPosition = camera.position;
                // The transpose of the projection matrix will be used to derive the frustum planes
                renderContext.projectionTranspose = camera.projectionTranspose;
                renderContext.zNear = BLITZEN_ZNEAR;

                // The draw count is passed again every frame even thought is is constant at the moment
                renderContext.drawCount = drawCount;
                renderContext.drawDistance = BLITZEN_DRAW_DISTANCE;

                // Debug values, controlled by inputs
                if(!pDebugValues)
                {
                    renderContext.debugPyramid = 0;// This does nothing now, something is broken with occlusion culling / debug pyramid
                    renderContext.occlusionEnabled = 1; // f3 to change
                    renderContext.lodEnabled = 1;// f4 to change
                }

                // Hardcoding the sun for now (this is used in the fragment shader but ignored)
                renderContext.sunlightDirection = BlitML::vec3(-0.57735f, -0.57735f, 0.57735f);
                renderContext.sunlightColor = BlitML::vec4(0.8f, 0.8f, 0.8f, 1.0f);

                // Let the renderere do its thing
                pVulkan->DrawFrame(renderContext);

                break;
            }
            default:
                break;
        }
    }
}