#include <D3d12.h>
#include <dxgi1_4.h>

#include "Core/blitLogger.h"

namespace BlitzenDX12
{
    #if !defined(NDEBUG)
        constexpr uint8_t ce_bDebugController = 1;
    #else
        constexpr uint8_t ce_bDebugController = 0;
    #endif
}