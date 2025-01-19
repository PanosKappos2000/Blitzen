#include "blitRenderer.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenEngine
{
    inline BlitzenVulkan::VulkanRenderer* gpVulkan = nullptr;
    inline void* gpDx12 = nullptr;

    inline Camera* gpMainCamera = nullptr;

    uint8_t CreateVulkanRenderer(BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer>& pVulkan, 
    uint32_t windowWidth, uint32_t windowHeight)
    {
        if(pVulkan.Data())
        {
            // Call the init function and store the result in the systems boolean for Vulkan
            uint8_t vulkanInitialized =  pVulkan.Data()->Init(windowWidth, windowHeight);
            gpVulkan = pVulkan.Data();
            return vulkanInitialized;
        }
        else
        {
            return 0;
        }
    }

    uint8_t isVulkanInitialized() { return gpVulkan != nullptr; }

    uint8_t CheckActiveRenderer(ActiveRenderer ar)
    {
        switch(ar)
        {
            case ActiveRenderer::Vulkan:
                return isVulkanInitialized();
            default:
                break;
        }
    }

    void SetupRequestedRenderersForDrawing(RenderingResources& resources, size_t drawCount)
    {
        if(gpVulkan)
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

            gpVulkan->SetupForRendering(vulkanData);
        }
    }

    void DrawFrame(Camera& camera, Camera* pMovingCamera, size_t drawCount, 
    uint32_t windowWidth, uint32_t windowHeight, uint8_t windowResize, 
    ActiveRenderer ar, RuntimeDebugValues* pDebugValues /*= nullptr*/)
    {
        // Check that the pointer for the active renderer is not Null, if it is warn the use that they should switch the active renderer
        if(!CheckActiveRenderer(ar))
        {
            BLIT_ERROR("Renderer not available, switch active renderer");
            return;
        }

        switch(ar)
        {
            case ActiveRenderer::Vulkan:
            {
                // Hardcoding the sun for now (this is used in the fragment shader but ignored)
                BlitML::vec3 sunDir(-0.57735f, -0.57735f, 0.57735f);
                BlitML::vec4 sunColor(0.8f, 0.8f, 0.8f, 1.0f);

                // Create the render context
                BlitzenVulkan::RenderContext renderContext(camera, pMovingCamera, drawCount, 
                sunDir, sunColor, windowResize);

                // Debug values, controlled by inputs
                if(pDebugValues)
                {
                    renderContext.debugPyramid = pDebugValues->isDebugPyramidActive; // f2 to change (inoperable)
                    renderContext.occlusionEnabled = pDebugValues->m_occlusionCulling; // f3 to change
                    renderContext.lodEnabled = pDebugValues->m_lodEnabled;// f4 to change
                }

                // Let Vulkan do its thing
                gpVulkan->DrawFrame(renderContext);

                break;
            }
            default:
                break;
        }
    }
}