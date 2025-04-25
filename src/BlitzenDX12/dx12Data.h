#pragma once 
#include <D3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <wrl/client.h>
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
    
    // Dx12 ignores the double buffering compile flag for now
    constexpr uint8_t ce_framesInFlight = 2;

    constexpr D3D_FEATURE_LEVEL Ce_DeviceFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	constexpr DXGI_FORMAT Ce_SwapchainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr DXGI_USAGE Ce_SwapchainBufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	constexpr DXGI_SWAP_EFFECT Ce_SwapchainSwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    struct Dx12Stats
    {
        uint8_t bDiscreteGPU = 0;

        uint8_t bResourceManagement = 0;
    };



    struct Swapchain
    {
        Microsoft::WRL::ComPtr<IDXGISwapChain3> comPrt;
        D3D12_RECT extent;

        UINT m_heapIncrement = 0;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_renderTargetViewDescriptor;
    };
}