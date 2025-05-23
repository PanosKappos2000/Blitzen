#pragma once
#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenWorld
{
    struct BlitzenPrivateContext
    {
        BlitzenCore::EngineState* pEngineState;
        BlitzenEngine::RendererPtrType pRenderer;
        BlitzenEngine::RenderingResources* pRenderingResources;
        BlitzenCore::EntityManager* pEntityMangager;

        void* pBlitzenContext;
    };
}