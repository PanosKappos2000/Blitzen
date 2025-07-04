#pragma once
#include "Core/blitzenEngine.h"

namespace BlitzenCore
{
    enum class BlitEventType : uint8_t
    {
        EngineShutdown = 0,
        RendererTransformUpdate = 1,
        WindowUpdate = 2,
        BringBackEditor = 3,
        BringDasherRuntimeDebugWindow = 4,
        FreezeFrustum = 5,
        HI_Z_MAP_levelDescrease = 6,
        HI_Z_MAP_levelIncrease = 7,

        MaxTypes = 8
    };
}