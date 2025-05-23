#pragma once
#include "Game/blitCamera.h"
#include "Core/Events/blitTimeManager.h"
#include "Renderer/Interface/blitRendererInterface.h"

namespace BlitzenWorld
{
    struct BlitzenWorldContext
    {
        // TODO:
        // This should be changed for client protection. Right now they can just break the engine by accessing these guys
        BlitzenEngine::CameraContainer* pCameraContainer;
        BlitzenCore::WorldTimerManager* pCoreClock;

        BlitzenEngine::RendererEvent rendererEvent;
    };
}