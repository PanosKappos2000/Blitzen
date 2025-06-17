#include "blitRenderer.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenEngine
{

    bool RenderingResourcesInit(RenderingResources* pResources, RendererPtrType pRenderer)
    {
        if(!pRenderer->UploadTexture("Assets/Textures/base_baseColor.dds"))
		{
			BLIT_ERROR("Rendering resources failed");
			return false;
		}
        
        // Does not return false by design, might change later.
        if (!pResources->m_textureManager.AddTexture(BlitzenCore::Ce_DefaultTextureName))
        {
            BLIT_ERROR("Something went wrong with texture map");
        }

        if (!pResources->m_textureManager.AddMaterial(0, 0, 0, 0, BlitzenCore::Ce_DefaultMaterialName))
        {
			BLIT_ERROR("Rendering resources failed");
            return false;
        }

        uint32_t bunnyMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/bunny.obj", BlitzenCore::Ce_DefaultMeshName) };
        if (bunnyMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
			BLIT_ERROR("Failed to load default bunny mesh");
            return false;
        }

        uint32_t kittenMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/kitten.obj", BlitzenCore::Ce_DefaultKittenMeshName) };
        if (kittenMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default kitten mesh");
            return false;
        }

        uint32_t dragonMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/dragon.obj", BlitzenCore::Ce_DefaultDragonMeshName) };
        if (dragonMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default dragon mesh");
            return false;
        }

        uint32_t humanMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/FinalBaseMesh.obj", BlitzenCore::Ce_DefaultHumanMeshname) };
        if (humanMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default human mesh");
            return false;
        }

#if defined(_WIN32) && !defined(BLIT_VK_FORCE) && !defined(BLIT_GL_LEGACY_OVERRIDE)
        pResources->m_meshContext.HLSL_TRIANGLES.ALLOC();
#endif

        // Success
        return true;
    }
}