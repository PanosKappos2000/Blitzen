#pragma once

#if defined(_WIN32)

#include <D3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <wrl/client.h>
#include <d3d12sdklayers.h>
#include <comdef.h>
#include "Core/blitzenEngine.h"
#include "BlitCL/blitzenContainerLibrary.h"
#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenDX12
{
    static_assert(sizeof(UINT) == sizeof(uint32_t));
    static_assert(sizeof(SIZE_T) == sizeof(size_t));

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

    constexpr D3D_FEATURE_LEVEL Ce_DeviceFeatureLevel = D3D_FEATURE_LEVEL_12_1;

	constexpr DXGI_FORMAT Ce_SwapchainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    constexpr DXGI_USAGE Ce_SwapchainBufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	constexpr DXGI_SWAP_EFFECT Ce_SwapchainSwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    constexpr DXGI_FORMAT Ce_DepthTargetFormat = DXGI_FORMAT_D32_FLOAT;

    constexpr FLOAT Ce_ClearDepth = 0.f;

    constexpr uint32_t Ce_DepthPyramidMaxMips = 16;
    constexpr DXGI_FORMAT Ce_DepthPyramidFormat = DXGI_FORMAT_R32_FLOAT;
    constexpr DXGI_FORMAT Ce_DepthTargetSRVFormat = DXGI_FORMAT_R32_FLOAT;


#if defined(DX12_OCCLUSION_DRAW_CULL) && defined(BLIT_DEPTH_PYRAMID_TEST)
    static_assert(Ce_SwapchainFormat == Ce_DepthPyramidFormat);
#else
    static_assert(Ce_DepthTargetSRVFormat == DXGI_FORMAT_R32_FLOAT);
    static_assert(Ce_DepthPyramidFormat == DXGI_FORMAT_R32_FLOAT);
#endif

    static_assert(Ce_SwapchainFormat == DXGI_FORMAT_R8G8B8A8_UNORM);
    static_assert(Ce_SwapchainSwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD);


    constexpr D3D12_TEXTURE_LAYOUT Ce_DefaultTextureFormat = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;


    // SHARED SRVs :
    constexpr UINT Ce_SharedSRVsRangeCount = 4;

    constexpr UINT Ce_SurfaceSRVRegister = 2;
    constexpr UINT Ce_SurfaceSRVRangeID = 0;
     
    constexpr UINT Ce_TransformSRVRegister = 3;
    constexpr UINT Ce_TransformSRVRangeID = 1;

    constexpr UINT Ce_RenderSRVRegister = 4;
    constexpr UINT Ce_RenderSRVRangeID = 2;

    constexpr UINT Ce_ViewCBVRegister = 0;
    constexpr UINT Ce_ViewCBVRootID = 3;


    // DESCRIPTORS FOR DRAW CULL :
    constexpr UINT Ce_DrawCullSRVsRangeCount = 3;

    constexpr UINT Ce_DrawCullDrawCmdUAVRegister = 0;
    constexpr UINT Ce_DrawCullDrawCmdUAVRangeID = 0;
    
    constexpr UINT Ce_DrawCullDrawCmdCountUAVRegister = 1;
    constexpr UINT Ce_DrawCullDrawCmdCountUAVRangeID = 1;

    constexpr UINT Ce_DrawCullLODSRVRegister = 7;
    constexpr UINT Ce_DrawCullLODSRVRangeID = 2;

    constexpr UINT Ce_DrawCullDrawCountContantRegister = 1;
	constexpr UINT Ce_DrawCullDrawCountContant32BitCount = 1;

    // Root param descriptor grouping
    constexpr uint32_t Ce_DrawCullRootParameterCount = 3;

    constexpr UINT Ce_DrawCullExclusiveSRVsRootID = 0;
    constexpr UINT Ce_DrawCullSharedSRVsRootID = 1;
    constexpr UINT Ce_DrawCullDrawCountConstantRootID= 2;

    // DESCRIPTORS FOR DRAW CULL INST :
    // additional descriptors
    constexpr UINT Ce_DrawCullInstSRVsRangeCount = 2;

    constexpr UINT Ce_DrawCullInstInstIdsxUAVRegister = 3;
	constexpr UINT Ce_DrawCullInstInstIdsxUAVRangeID = 0;

    constexpr UINT Ce_DrawCullInstInstCounterUAVRegister = 2;
	constexpr UINT Ce_DrawCullInstInstCounterUAVRangeID = 1;

	// Root param descriptor grouping
	constexpr UINT Ce_DrawCullInstRootParameterCount = 4;

	constexpr UINT Ce_DrawCullInstExclusiveSRVsRootID = 0;
	constexpr UINT Ce_DrawCullInstSharedSRVsRootID = 1;
	constexpr UINT Ce_DrawCullInstDrawCountConstantRootID = 2;
	constexpr UINT Ce_DrawCullInstAdditionalSRVsRootID = 3;

    // DESCRIPTORS FOR DRAW OCC FIRST:
	// additional SRVs
    constexpr UINT Ce_DrawOccFirstDrawVisUAVRegister = 5;
    
    // Root param dscriptor grouping
    constexpr UINT Ce_DrawOccFirstRootParameterCount = 4;

	constexpr UINT Ce_DrawOccFirstExclusiveSRVsRootId = 0;
	constexpr UINT Ce_DrawOccFirstSharedSRVsRootId = 1;
	constexpr UINT Ce_DrawOccFirstDrawCountRootId = 2;
	constexpr UINT Ce_DrawOccFirstDrawVisUAVRootId = 3;

    // DESCRIPTORS FOR HI_Z_MAP GENERATION: 
    constexpr UINT Ce_HI_Z_MapUAVRegister = 0;

    constexpr UINT Ce_HI_Z_MapSRVRegister = 0;

    constexpr UINT Ce_HI_Z_MapMipLvlConstantRegister = 0;
    constexpr UINT Ce_HI_Z_MapMipLvlContant32BitCount = 1;

	// Root param descriptor grouping
    constexpr UINT Ce_HI_Z_MapRootParameterCount = 3;

    constexpr UINT Ce_HI_Z_MapUAVRootID = 0;
    constexpr UINT Ce_HI_Z_MapSRVRootID = 1;
    constexpr UINT Ce_HI_Z_MapMipLvlConstantRootID = 2;

    // DESCRIPTOR FOR DRAW OCC LATE:
    // additional SRVs
    constexpr UINT Ce_DrawOccLateDrawVisUAVRegister = 5;

    // HI_Z MAP For occlusion
    constexpr UINT Ce_DrawOccLateHI_Z_MapSRVRegister = 10;

	// Root param descriptor grouping
	constexpr UINT Ce_DrawOccLateRootParameterCount = 5;
    
	constexpr UINT Ce_DrawOccLateExclusiveSRVsRootId = 0;
	constexpr UINT Ce_DrawOccLateSharedSRVsRootId = 1;
	constexpr UINT Ce_DrawOccLateDrawCountRootId = 2;
	constexpr UINT Ce_DrawOccLateHI_Z_MapRootId = 3;
    constexpr UINT Ce_DrawOccLateDrawVisUAVRootId = 4;

    // DESCRIPTORS FOR DRAW OCC TEMPORAL:
    constexpr UINT Ce_DrawOccTemporalHI_Z_MapSRVRegister = 10;

	// Root parameter descriptor grouping
	constexpr UINT Ce_DrawOccTemporalRootParameterCount = 4;
	constexpr UINT Ce_DrawOccTemporalExclusiveSRVsRootId = 0;
	constexpr UINT Ce_DrawOccTemporalSharedSRVsRootId = 1;
	constexpr UINT Ce_DrawOccTemporalDrawCountRootId = 2;
	constexpr UINT Ce_DrawOccTemporalHI_Z_MapRootId = 3;

    // DESCRIPTORS FOR OPAQUE DRAW :
    // exclusive range specific
    constexpr UINT Ce_OpaqueDrawExclusiveSRVsRangeCount = 1;
    constexpr UINT Ce_OpaqueDrawVtxxSRVRegister = 0;
    constexpr UINT Ce_OpaqueDrawVtxSRVRangeId = 0;

    // texture sampler
    constexpr UINT Ce_OpaqueDrawTexSMPRegister = 0;

    // material buffer
    constexpr UINT Ce_OpaqueDrawMatSRVRegister = 5;

    // object id root constant
    constexpr UINT Ce_OpaqueDrawObjIDConstantRegister = 1;
    constexpr UINT Ce_OpaqueDrawObjIDConstant32BitCount = 1;

    // texture descriptor array
    constexpr UINT Ce_OpaqueDrawTexRegister = 8;
    constexpr UINT Ce_OpaqueDrawTexDescriptorCount = BlitzenCore::Ce_MaxTextureCount;

    // Root param descriptor grouping 
    constexpr UINT Ce_OpaqueDrawRootParameterCount = 6;
    constexpr UINT Ce_OpaqueDrawExclusiveSRVsRootID = 0;
    constexpr UINT Ce_OpaqueDrawSharedSRVsRootID = 1;
    constexpr UINT Ce_OpaqueDrawObjIDRootID = 2;
    constexpr UINT Ce_OpaqueDrawTexSMPRootID = 3;
    constexpr UINT Ce_OpaqueDrawMatSRVRootID = 4;
    constexpr UINT Ce_OpaqueDrawTexSRVRootID = 5;

    // DESCRIPTORS FOR OPAQUE DRAW INST :
    // additional srvs
    constexpr UINT Ce_OpaqueDrawInstAdditionalSRVsRangeCount = 1;
    constexpr UINT Ce_OpaqueDrawInstInstUAVRegister = 3;
    constexpr UINT Ce_OpaqueDrawInstInstSRVRangeID = 0;

    // Root param descriptor grouping 
    constexpr UINT Ce_OpaqueDrawInstRootParameterCount = 7;
    constexpr UINT Ce_OpaqueDrawInstExclusiveSRVsRootID = 0;
    constexpr UINT Ce_OpaqueDrawInstSharedSRVsRootID = 1;
    constexpr UINT Ce_OpaqueDrawInstObjIDRootID = 2;
    constexpr UINT Ce_OpaqueDrawInstTexSMPRootID = 3;
    constexpr UINT Ce_OpaqueDrawInstMatSRVRootID = 4;
    constexpr UINT Ce_OpaqueDrawInstTexSRVRootID = 5;
    constexpr UINT Ce_OpaqueDrawInstInstSRVRootID = 6;

    constexpr uint32_t Ce_SrvDescriptorCount = (Ce_OpaqueDrawExclusiveSRVsRangeCount + 
        Ce_SharedSRVsRangeCount + 
        Ce_DrawCullSRVsRangeCount +
		Ce_DrawCullInstSRVsRangeCount +
        1 + // Visiblity buffer 
        Ce_OpaqueDrawInstAdditionalSRVsRangeCount +
        Ce_DepthPyramidMaxMips) * ce_framesInFlight;// Double or triple buffering

    constexpr UINT Ce_SamplerDescriptorCount = 2; // Depth pyramid and texture samplers


    /* SSBO data copy helpers */
    constexpr UINT Ce_ConstDataSSBOCount = 6;
    constexpr UINT Ce_VertexStagingBufferIndex = 0;
    constexpr UINT Ce_IndexStagingBufferIndex = 1;
    constexpr UINT Ce_SurfaceStagingBufferIndex = 2;
    constexpr UINT Ce_RenderStagingBufferIndex = 3;
    constexpr UINT Ce_LodStagingIndex = 4;
    constexpr UINT Ce_MaterialStagingIndex = 5;
    
    constexpr UINT Ce_VarBuffersCount = 3 * ce_framesInFlight;

    constexpr UINT Ce_VarSSBODataCount = 1;

    constexpr UINT Ce_TransformStagingBufferIndex = 0;

    // Buffer size used for texture staging buffer
    constexpr SIZE_T Ce_TextureDataStagingSize = 128 * 1024 * 1024;

// OCCLUSION MODES
#if defined(DX12_TEMPORAL_DRAW_OCCLUSION)
    constexpr uint8_t CE_DX12TEMPORAL_OCCLUSION = 1;
    constexpr uint8_t CE_DX12OCCLUSION = 1;
#elif defined(DX12_OCCLUSION_DRAW_CULL)
    constexpr uint8_t CE_DX12OCCLUSION = 1;
    constexpr uint8_t CE_DX12TEMPORAL_OCCLUSION = 0;
#else
    constexpr uint8_t CE_DX12OCCLUSION = 0;
    constexpr uint8_t CE_DX12TEMPORAL_OCCLUSION = 0;
#endif


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

    struct DX2DTEX
    {
        DX12WRAPPER<ID3D12Resource> resource;
        UINT mipLevels{ 0 };
        DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
        D3D12_GPU_DESCRIPTOR_HANDLE view;
    };

    struct DepthPyramid
    {
        DX12WRAPPER<ID3D12Resource> pyramid;
        uint32_t width{ 0 };
        uint32_t height{ 0 };

        UINT mipCount{ 0 };
        D3D12_GPU_DESCRIPTOR_HANDLE mips[Ce_DepthPyramidMaxMips];
    };



   
    // Draw Indirect command struct (passed to the shaders)
    struct IndirectDrawCmd
    {
        // Id into the render object buffer(root constant)
        uint32_t objId;

        // Draw command
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

#endif