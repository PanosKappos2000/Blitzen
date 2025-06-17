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

        MaxTypes = 8
    };
}