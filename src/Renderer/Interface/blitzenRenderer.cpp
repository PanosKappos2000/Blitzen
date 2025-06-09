#define CGLTF_IMPLEMENTATION
#include "blitRenderer.h"
#include "Renderer/Entities/Interface/blitEntityInterface.h"

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

        if (!LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/bunny.obj", BlitzenCore::Ce_DefaultMeshName))
        {
			BLIT_ERROR("Rendering resources failed");
            return false;
        }

        // Success
        return true;
    }
}