#pragma once
#include "Game/blitCamera.h"
#include "Core/Events/blitTimeManager.h"
#include "Renderer/Interface/blitRendererInterface.h"

namespace BlitzenWorld
{
    struct BlitzenWorldContext
    {
        BlitzenEngine::CameraContainer* pCameraContainer;
        BlitzenCore::WorldTimerManager* pCoreClock;

        BlitzenEngine::RendererEvent rendererEvent;
    };
}