#pragma once
#include "Core/blitzenEngine.h"
#include "Renderer/Resources/blitRenderingResources.h"
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
        BlitzenEngine::Camera* pCamera;

        RenderingResources* pResources;

        inline DrawContext(BlitzenEngine::Camera* pCam, RenderingResources* pr): pCamera(pCam), pResources(pr) 
        {

        }
    };
}