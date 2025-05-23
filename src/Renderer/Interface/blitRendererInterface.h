#pragma once
#include "Core/blitzenEngine.h"
#include "Renderer/Resources/RenderObject/blitRender.h"
#include "Game/blitCamera.h" 

namespace BlitzenEngine
{
    enum class RendererEvent : uint8_t
    {
        RENDERER_TRANSFORM_UPDATE = 0,

        MAX_RENDERER_EVENTS
    };

    struct DrawContext
    {
        Camera& m_camera;
        MeshResources& m_meshes;
        RenderContainer& m_renders;
        TextureManager& m_textures;

        DrawContext(Camera& camera, MeshResources& meshes, RenderContainer& renders, TextureManager& textureManager) :m_camera(camera), m_meshes(meshes), m_renders(renders), m_textures{ textureManager }
		{

		}
    };
}