#pragma once
#include "Core/blitzenCore.h"
#include "Game/blitzenCamera.h"

namespace BlitzenWorld
{
    inline BlitzenEngine::Camera* GetMainCamera(void** ppContext)
    {
        return reinterpret_cast<BlitzenEngine::CameraContainer*>(ppContext[BlitzenCore::Ce_WorldContextCameraId])->GetCameraContainer();
    }
}