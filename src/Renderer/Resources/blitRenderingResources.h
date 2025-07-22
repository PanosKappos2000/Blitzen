#pragma once
#include "Mesh/blitMeshes.h"
#include "Textures/blitTextures.h"
#include "RapidFile/blitResourceRPF.h"
#include "Renderer/Resources/Terrain/blitTerrain.h"
#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenEngine
{
    // Rendering resources container
    class RenderingResources
    {
    public:
        RenderingResources operator = (const RenderingResources& rr) = delete;
        RenderingResources operator = (RenderingResources& rr) = delete;

        MeshResources m_meshContext;
        TextureManager m_textureManager;
        TerrainContainer m_terrainContainer;

        RenderingLoadingContextMesh mLoadingContextMesh;

        BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE m_mappedFile;
    };

    // Copies vertex data and their indices for a single mesh to the staging buffer.
    // It resets the count of vertices and indices for the next mesh, but it keeps a map count.
    bool CopyMeshResourcesToStagingBuffer(MeshResources* pMeshes, RenderingLoadingContextMesh& loadingContextMesh);
}