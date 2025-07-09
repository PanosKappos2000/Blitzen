#pragma once
#include "Mesh/blitMeshes.h"
#include "Textures/blitTextures.h"
#include "RapidFile/blitResourceRPF.h"
#include "Renderer/Resources/Terrain/blitTerrain.h"

namespace BlitzenEngine
{
    // Rendering resources container
    struct RenderingResources
    {
        RenderingResources operator = (const RenderingResources& rr) = delete;
        RenderingResources operator = (RenderingResources& rr) = delete;

        MeshResources m_meshContext;
        TextureManager m_textureManager;
        TerrainContainer m_terrainContainer;

        BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE m_mappedFile;
    };
}