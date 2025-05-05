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

    constexpr DXGI_FORMAT Ce_DepthTargetFormat = DXGI_FORMAT_D32_FLOAT;
    constexpr FLOAT Ce_ClearDepth = 0.f;


    /* Srv desriptors for both compute and graphics pipelines*/
    constexpr uint32_t Ce_SharedSrvRangeCount = 5;

    constexpr UINT Ce_SurfaceBufferRegister = 2;
    constexpr UINT Ce_SurfaceBufferDescriptorCount = 1;
    constexpr UINT Ce_SurfaceBufferRangeElement = 0;
     
    constexpr UINT Ce_TransformBufferRegister = 3;
    constexpr UINT Ce_TransformBufferDescriptorCount = 1;
    constexpr UINT Ce_TransformBufferRangeElement = 1;

    constexpr UINT Ce_RenderObjectBufferRegister = 4;
    constexpr UINT Ce_RenderObjectBufferDescriptorCount = 1;
    constexpr UINT Ce_RenderObjectBufferRangeElement = 2;

    constexpr UINT Ce_IndirectDrawBufferRegister = 0;// First Uav
    constexpr UINT Ce_IndirectDrawBufferDescriptorCount = 1;
    constexpr UINT Ce_IndirectDrawBufferRangeElement = 3;

    constexpr UINT Ce_ViewDataBufferRegister = 0;// First Cbv
    constexpr UINT Ce_ViewDataBufferDescriptorCount = 1;
    constexpr UINT Ce_ViewDataBufferRangeElement = 4;

    /* Srv Descriptors for compute pipeline */
    constexpr uint32_t Ce_CullSrvRangeCount = 2;
    
    constexpr UINT Ce_IndirectCountBufferRegister = 1;
    constexpr UINT Ce_IndirectCountBufferDescriptorCount = 1;
    constexpr UINT Ce_IndirectCountBufferRangeElement = 0;

    constexpr UINT Ce_LODBufferRegister = 7;
    constexpr UINT Ce_LODBufferDescriptorCount = 1;
    constexpr UINT Ce_LODBufferRangeElement = 1;

    /* SSBO descriptors for main graphics pipeline */
    constexpr uint32_t Ce_OpaqueSrvRangeCount = 1;

    constexpr UINT Ce_VertexBufferRegister = 0;
	constexpr UINT Ce_VertexBufferDescriptorCount = 1;
    constexpr UINT Ce_VertexBufferRangeElement = 0;

    constexpr uint32_t Ce_SrvDescriptorCount = (Ce_OpaqueSrvRangeCount + Ce_SharedSrvRangeCount + Ce_CullSrvRangeCount) * ce_framesInFlight;// Double or triple buffering

    
    /* SSBO data copy helpers */
    constexpr UINT Ce_ConstDataSSBOCount = 5;
    constexpr UINT Ce_VertexStagingBufferIndex = 0;
    constexpr UINT Ce_IndexStagingBufferIndex = 1;
    constexpr UINT Ce_SurfaceStagingBufferIndex = 2;
    constexpr UINT Ce_RenderStagingBufferIndex = 3;
    constexpr UINT Ce_LodStagingIndex = 4;

    constexpr UINT Ce_VarSSBODataCount = 1;
    constexpr UINT Ce_TransformStagingBufferIndex = 0;


    constexpr SIZE_T Ce_TextureDataStagingSize = 128 * 1024 * 1024;



    struct Dx12Stats
    {
        uint8_t bDiscreteGPU = 0;

        uint8_t bResourceManagement = 0;
    };

    template<typename HANDLE>
	using DX12WRAPPER = Microsoft::WRL::ComPtr<HANDLE>;

    template<typename DATA>
    struct CBuffer
    {
        DX12WRAPPER<ID3D12Resource> buffer;
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};

        DATA* pData{ nullptr };
    };

    struct SSBO
    {
        DX12WRAPPER<ID3D12Resource> buffer{ nullptr };
        SIZE_T heapOffset[ce_framesInFlight]{};
    };

    struct VarSSBO
    {
        DX12WRAPPER<ID3D12Resource> buffer{ nullptr };
        SIZE_T heapOffset{};

        DX12WRAPPER<ID3D12Resource> staging{ nullptr };
        void* pData{ nullptr };
        size_t dataCopySize{ 0 };
    };



    // Draw Indirect command struct (passed to the shaders)
    struct IndirectDrawCmd
    {
        uint32_t drawId;
        D3D12_DRAW_INDEXED_ARGUMENTS command;// 5 32bit integers

        uint32_t padding0;
        uint32_t padding1;
    };
    static_assert(sizeof(IndirectDrawCmd) % 16 == 0);
    constexpr uint32_t Ce_IndirectDrawCmdBufferSize = 1'000'000;



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