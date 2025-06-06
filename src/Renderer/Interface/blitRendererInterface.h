#pragma once
#include "Core/blitzenEngine.h"
#include "Renderer/Resources/RenderObject/blitRender.h"
#include "Renderer/View/blitCamera.h"
#include "Platform/blitPlatformContext.h"

namespace BlitzenEngine
{
    enum class RendererEvent : uint8_t
    {
        RENDERER_TRANSFORM_UPDATE = 0,
        RENDERER_WINDOW_RESIZE = 1,

        MAX_RENDERER_EVENTS
    };

    struct DrawContext
    {
        Camera& m_camera;
        MeshResources& m_meshes;
        RenderContainer& m_renders;
        TextureManager& m_textures;
		BlitzenPlatform::PlatformContext* m_pPlatform{ nullptr };

        DrawContext(Camera& camera, MeshResources& meshes, RenderContainer& renders, TextureManager& textureManager, BlitzenPlatform::PlatformContext* pPlatform)
            :m_camera(camera), m_meshes(meshes), m_renders(renders), m_textures{ textureManager }, m_pPlatform{pPlatform}
		{

		}
    };
}