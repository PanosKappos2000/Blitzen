#pragma once
#include "Renderer/blitRenderer.h"

namespace BlitzenWorld
{
    struct BlitzenPrivateContext
    {
        BlitzenCore::EngineState* pEngineState;
        BlitzenEngine::RendererPtrType pRenderer;

        void* pBlitzenContext;
    };
}