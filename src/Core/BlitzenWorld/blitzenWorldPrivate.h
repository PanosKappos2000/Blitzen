#pragma once
#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenWorld
{
    struct BlitzenPrivateContext
    {
        BlitzenCore::EngineState* pEngineState;
        BlitzenEngine::RendererPtrType pRenderer;

        void* pBlitzenContext;
    };
}