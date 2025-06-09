#pragma once
#include "Renderer/View/blitCamera.h"
#include "Core/Events/blitTimeManager.h"
#include "Renderer/Interface/blitRendererInterface.h"

namespace BlitzenWorld
{
    struct BlitzenWorldContext
    {
        BlitzenEngine::CameraContainer* pCameraContainer;
        // TODO: MOVE TIME MANAGER TO PRIVATE
        BlitzenCore::WorldTimeManager* pCoreClock;
        
        float deltaTime;
    };
}