#pragma once
#include "Renderer/Interface/blitRenderer.h"
#include "Core/Dasher/Interface/dasherInterface.h"

namespace BlitzenWorld
{
    struct BlitzenPrivateContext
    {
        BlitzenCore::EngineState* pEngineState{ nullptr };
        BlitzenEngine::RendererPtrType pRenderer{ nullptr };
        BlitzenEngine::RenderingResources* pRenderingResources{ nullptr };
        BlitzenEngine::EntityManager* pEntityMangager{ nullptr };
        BlitzenPlatform::PlatformContext* pPlatform{ nullptr };
        BlitzenCore::Dasher* pDasher{ nullptr };

        void* pBlitzenContext;
    };
}