#pragma once
#include "Game/blitCamera.h"
#include "Core/blitTimeManager.h"
#include "Renderer/Interface/blitRendererInterface.h"

namespace BlitzenWorld
{
    struct BlitzenWorldContext
    {
        BlitzenEngine::CameraContainer* pCameraContainer;
        BlitzenCore::WorldTimerManager* pCoreClock;
        BlitzenEngine::RenderContainer* pRenders;
        BlitzenEngine::MeshResources* pMeshes;

        BlitzenEngine::RendererEvent rendererEvent;
        BlitzenEngine::RendererTransformUpdateContext rendererTransformUpdate;
    };
}