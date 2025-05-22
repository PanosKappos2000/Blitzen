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

    struct RendererTransformUpdateContext
    {
        uint32_t transformId;
        MeshTransform* pTransform{ nullptr };
    };

    struct DrawContext
    {
        Camera& m_camera;
        MeshResources& m_meshes;
        RenderContainer& m_renders;
        Material* pMaterials;
        uint32_t materialCount;

		DrawContext(Camera& camera, MeshResources& meshes, RenderContainer& renders) :m_camera(camera), m_meshes(meshes), m_renders(renders)
		{

		}
    };
}