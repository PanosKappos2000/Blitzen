#include <D3d12.h>
#include <dxgi1_4.h>

#include "Core/blitzenContainerLibrary.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/blitLogger.h"

namespace BlitzenDX12
{
    #if !defined(NDEBUG)
        constexpr uint8_t ce_bDebugController = 1;
    #else
        constexpr uint8_t ce_bDebugController = 0;
    #endif

    #if !defined(BLIT_DOUBLE_BUFFERING)
        constexpr uint8_t ce_framesInFlight = 1;
    #else
        constexpr uint8_t ce_frameInFlight = 2;
    #endif

    struct Dx12Stats
    {
        uint8_t ce_bDiscreteGPU = 0;

        uint8_t ce_bResourceManagement = 0;
    };
}