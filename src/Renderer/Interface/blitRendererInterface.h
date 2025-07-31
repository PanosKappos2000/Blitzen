#pragma once
#include "Core/blitzenEngine.h"
#include "Renderer/Entities/Residents/blitResidentManager.h"
#include "Renderer/View/blitCamera.h"
#include "Platform/blitPlatformContext.h"
#include "Renderer/Resources/Mesh/blitMeshes.h"
#include "Renderer/Resources/Terrain/blitTerrain.h"
#include "Renderer/Resources/Textures/blitTextures.h"
#include "Renderer/Resources/Materials/blitMaterial.h"

namespace BlitzenEngine
{
    struct DrawContext
    {
        Camera& m_camera;
        MeshResources& m_meshes;
        WORLD_RESIDENTS* m_pResidents;
        MaterialManager* pMatManager{ nullptr };
        TextureManager& m_textures;
		BlitzenPlatform::PlatformContext* m_pPlatform{ nullptr };
        TerrainContainer* m_pTerrain{ nullptr };

        DrawContext(Camera& camera, MeshResources& meshes, TextureManager& textureManager, BlitzenPlatform::PlatformContext* pPlatform)
            :m_camera(camera), m_meshes(meshes), m_textures{ textureManager }, m_pPlatform{pPlatform}
		{

		}
    };
}