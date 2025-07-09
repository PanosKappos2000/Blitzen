#pragma once
#include "Core/blitzenEngine.h"
#include "Renderer/Entities/Residents/blitResidentManager.h"
#include "Renderer/Resources/blitRenderingResources.h"
#include "Renderer/View/blitCamera.h"
#include "Platform/blitPlatformContext.h"

namespace BlitzenEngine
{
    struct DrawContext
    {
        Camera& m_camera;
        MeshResources& m_meshes;
        WORLD_RESIDENTS* m_pResidents;
        TextureManager& m_textures;
		BlitzenPlatform::PlatformContext* m_pPlatform{ nullptr };
        TerrainContainer* m_pTerrain{ nullptr };

        DrawContext(Camera& camera, MeshResources& meshes, TextureManager& textureManager, BlitzenPlatform::PlatformContext* pPlatform)
            :m_camera(camera), m_meshes(meshes), m_textures{ textureManager }, m_pPlatform{pPlatform}
		{

		}
    };
}