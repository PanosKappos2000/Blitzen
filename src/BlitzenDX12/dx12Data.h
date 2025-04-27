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
        constexpr uint8_t Ce_GPUValidationRequested = 0;
    #endif
    
    // Dx12 ignores the double buffering compile flag for now
    constexpr uint8_t ce_framesInFlight = 2;

    constexpr D3D_FEATURE_LEVEL Ce_DeviceFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	constexpr DXGI_FORMAT Ce_SwapchainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr DXGI_USAGE Ce_SwapchainBufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	constexpr DXGI_SWAP_EFFECT Ce_SwapchainSwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    constexpr uint32_t Ce_OpaqueGraphicsRangeCount = 5;

    constexpr UINT Ce_VertexBufferRegister = 0;
	constexpr UINT Ce_VertexBufferDescriptorCount = 1;

    constexpr UINT Ce_ConstDataSSBOCount = 1;

    constexpr UINT Ce_VertexStagingBufferIndex = 0;

    struct Dx12Stats
    {
        uint8_t bDiscreteGPU = 0;

        uint8_t bResourceManagement = 0;
    };

    template<typename HANDLE>
	using DX12WRAPPER = Microsoft::WRL::ComPtr<HANDLE>;

    struct SSBO
    {
        DX12WRAPPER<ID3D12DescriptorHeap> srvHeap{};
        DX12WRAPPER<ID3D12Resource> buffer{ nullptr };
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    };

	template<typename DATA>
    struct VarSSBO
    {
        DX12WRAPPER<ID3D12DescriptorHeap> srvHeap{};
        DX12WRAPPER<ID3D12Resource> buffer{ nullptr };
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

        DX12WRAPPER<ID3D12Resource> staging{ nullptr };
        DATA* pData{ nullptr };
    };




    // Useful helper to check for device removal before calling a function that uses it
    inline uint8_t CheckForDeviceRemoval(ID3D12Device* device)
    {
        auto removalReason = device->GetDeviceRemovedReason();
        if (FAILED(removalReason))
        {
            _com_error err{ removalReason };
            BLIT_FATAL("Device removal reason: %s", err.ErrorMessage());
            return 0;
        }

        // Safe
        return 1;
    }

    // If a dx12 functcion fails, it can calls this to log the result and return 0
    inline uint8_t LOG_ERROR_MESSAGE_AND_RETURN(HRESULT res)
    {
        _com_error err{ res };
        BLIT_ERROR("Dx12Error: %s", err.ErrorMessage());
        return 0;
    }
}