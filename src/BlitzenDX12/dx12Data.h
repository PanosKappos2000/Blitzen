#include <D3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <d3d12sdklayers.h>
#include <comdef.h>
#include "Engine/blitzenEngine.h"
#include "BlitCL/blitzenContainerLibrary.h"
#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenDX12
{
    #if !defined(NDEBUG)
        constexpr uint8_t ce_bDebugController = 1;
        #if defined(DX12_ENABLE_GPU_BASED_VALIDATION)
            constexpr uint8_t Ce_GPUValidationRequested = 1;
        #else
            constexpr uint8_t Ce_GPUValidationRequested = 0;
        #endif
    #else
        constexpr uint8_t ce_bDebugController = 0;
    #endif


    #if !defined(BLIT_DOUBLE_BUFFERING)
        constexpr uint8_t ce_framesInFlight = 1;
    #else
        constexpr uint8_t ce_frameInFlight = 2;
    #endif

    constexpr D3D_FEATURE_LEVEL Ce_DeviceFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    struct Dx12Stats
    {
        uint8_t bDiscreteGPU = 0;

        uint8_t bResourceManagement = 0;
    };

    struct Swapchain
    {
        Microsoft::WRL::ComPtr<IDXGISwapChain1> handle;
        D3D12_RECT extent;

        UINT m_heapIncrement = 0;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_renderTargetViewDescriptor;
    };
}