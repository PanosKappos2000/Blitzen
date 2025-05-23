#pragma once
#include "Mesh/blitMeshes.h"
#include "Textures/blitTextures.h"



namespace BlitzenEngine
{
    // Rendering resources container
    struct RenderingResources
    {
        RenderingResources operator = (const RenderingResources& rr) = delete;
        RenderingResources operator = (RenderingResources& rr) = delete;

        MeshResources m_meshContext;

        TextureManager m_textureManager;
    };
    
}