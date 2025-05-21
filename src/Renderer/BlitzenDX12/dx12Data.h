#pragma once 
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


    /* SHARED DESCRIPTORS FOR COMPUTE AND GRAPHICS SHADERS */
    constexpr uint32_t Ce_SharedSrvRangeCount = 4;

#if defined(DX12_TEMPORAL_DRAW_OCCLUSION)
    constexpr uint32_t Ce_AdditionalSharedSRVs = 0;
#elif defined(DX12_OCCLUSION_DRAW_CULL)
    constexpr uint32_t Ce_AdditionalSharedSRVs = 0;
#elif defined(BLITZEN_DRAW_INSTANCED_CULLING)
    constexpr uint32_t Ce_AdditionalSharedSRVs = 1;
#else
    constexpr uint32_t Ce_AdditionalSharedSRVs = 0;
#endif

    constexpr UINT Ce_SurfaceBufferRegister = 2;
    constexpr UINT Ce_SurfaceBufferDescriptorCount = 1;
    constexpr UINT Ce_SurfaceBufferRangeElement = 0;
     
    constexpr UINT Ce_TransformBufferRegister = 3;
    constexpr UINT Ce_TransformBufferDescriptorCount = 1;
    constexpr UINT Ce_TransformBufferRangeElement = 1;

    constexpr UINT Ce_RenderObjectBufferRegister = 4;
    constexpr UINT Ce_RenderObjectBufferDescriptorCount = 1;
    constexpr UINT Ce_RenderObjectBufferRangeElement = 2;

    constexpr UINT Ce_ViewDataBufferRegister = 0;// First Cbv
    constexpr UINT Ce_ViewDataBufferDescriptorCount = 1;
    constexpr UINT Ce_ViewDataBufferRangeElement = 3;

    constexpr UINT Ce_InstanceBufferRegister = 3;
    constexpr UINT Ce_InstanceBufferDescriptorCount = 1;
    constexpr UINT Ce_InstanceBufferRangeElement = 4;



    /* DESCRIPTORS FOR culling shaders */
    constexpr uint32_t Ce_CullSrvRangeCount = 3;

#if defined(DX12_TEMPORAL_DRAW_OCCLUSION)
    constexpr uint32_t Ce_AdditionalCullSRVs = 2 + Ce_DepthPyramidMaxMips;// depth target srv + Hi-z Map + Hi-z Mip UAVs. No draw visibility buffer
#elif defined(DX12_OCCLUSION_DRAW_CULL)
    constexpr uint32_t Ce_AdditionalCullSRVs = 3 + Ce_DepthPyramidMaxMips;// Draw visibility + depth target srv + Hi-z Map + Hi-z Mip UAVs
#elif defined(BLITZEN_DRAW_INSTANCED_CULLING)
    constexpr uint32_t Ce_AdditionalCullSRVs = 1;
#else
    constexpr uint32_t Ce_AdditionalCullSRVs = 0;
#endif

    constexpr UINT Ce_IndirectDrawBufferRegister = 0;// First Uav
    constexpr UINT Ce_IndirectDrawBufferDescriptorCount = 1;
    constexpr UINT Ce_IndirectDrawBufferRangeElement = 0;
    
    constexpr UINT Ce_IndirectCountBufferRegister = 1;
    constexpr UINT Ce_IndirectCountBufferDescriptorCount = 1;
    constexpr UINT Ce_IndirectCountBufferRangeElement = 1;

    constexpr UINT Ce_LODBufferRegister = 7;
    constexpr UINT Ce_LODBufferDescriptorCount = 1;
    constexpr UINT Ce_LODBufferRangeElement = 2;

    constexpr UINT Ce_CullShaderRootConstantRegister = 2;

    // ADDITIONAL DESCRIPTORS FOR INSTANCED MODE
    constexpr UINT Ce_LODInstanceBufferRegister = 2;
    constexpr UINT Ce_LODInstanceBufferDescriptorCount = 1;

    // ADDITIONAL DESCRIPTORS FOR OCCLUSION MODE
    constexpr UINT Ce_DrawVisibilityBufferRegister = 5;
    constexpr UINT Ce_DrawVisibilityBufferDescriptorCount = 1;

    constexpr UINT Ce_DepthPyramidCullRegister = 10;
    constexpr UINT Ce_DepthPyramidCullDescriptorCount = 1;

    // ROOT PARAMETERS
    constexpr uint32_t Ce_CullRootParameterCount = 3;

    constexpr UINT Ce_CullExclusiveSRVsParameterId = 0;
    constexpr UINT Ce_CullSharedSRVsParameterId = 1;
    constexpr UINT Ce_CullDrawCountParameterId = 2;

    // ROOT PARAMETER OCCLUSION LATE PASS
    constexpr UINT Ce_DrawOccLateRootParameterCount = 4;
    constexpr uint32_t Ce_DrawOccLateDepthPyramidParameterId = 3;



    /* DESCRIPTORS FOR opaqueDraw pipeline */
    constexpr uint32_t Ce_OpaqueSrvRangeCount = 1;

    constexpr UINT Ce_VertexBufferRegister = 0;
	constexpr UINT Ce_VertexBufferDescriptorCount = 1;
    constexpr UINT Ce_VertexBufferRangeElement = 0;

    constexpr UINT Ce_SamplerCount = 1;
    constexpr UINT Ce_DefaultTextureSamplerRegister = 0;
    constexpr UINT Ce_DefaultTextureSamplerDescriptorCount = 1;

    constexpr UINT Ce_MaterialBufferRegister = 5;
    constexpr UINT Ce_MaterialBufferDescriptorCount = 1;
    constexpr UINT Ce_MaterialBufferRangeElement = 3;

    constexpr UINT Ce_ObjectIdRegister = 1;

    constexpr UINT Ce_TextureDescriptorsRegister = 8;
    constexpr UINT Ce_TextureDescriptorCount = BlitzenCore::Ce_MaxTextureCount;

    // ROOT PARAMETERS
    constexpr uint32_t Ce_OpaqueRootParameterCount = 6;

    constexpr UINT Ce_OpaqueExclusiveBuffersElement = 0;
    constexpr UINT Ce_OpaqueSharedBuffersElement = 1;
    constexpr UINT Ce_OpaqueObjectIdElement = 2;
    constexpr UINT Ce_OpaqueSamplerElement = 3;
    constexpr UINT Ce_MaterialSrvElement = 4;
    constexpr UINT Ce_TextureDescriptorsElement = 5;

    /* DESCRIPTORS FOR DEPTH PYRAMID GENERATION SHADER */
    constexpr UINT Ce_DepthPyramidGenUAVRegister = 0;
    constexpr UINT Ce_DepthPyramidGenUAVDescriptorCount = 1;

    constexpr UINT Ce_DepthPyramidSRVRegister = 0;
    constexpr UINT Ce_DepthPyramidSRVDescriptorCount = 1;

    constexpr UINT Ce_DepthPyramidRootConstantsRegister = 0;
    constexpr UINT Ce_DepthPyramidRootConstantsCount = 1;

    // ROOT PARAMETERS
    constexpr UINT Ce_DepthPyramidParameterCount = 3;

    constexpr UINT Ce_DepthPyramidSRVRootParameterId = 0;
    constexpr UINT Ce_DepthPyramidUAVRootParameterId = 1;
    constexpr UINT Ce_DepthPyramidRootConstantParameterId = 2;

    constexpr uint32_t Ce_SrvDescriptorCount = (Ce_OpaqueSrvRangeCount + Ce_SharedSrvRangeCount + Ce_CullSrvRangeCount + Ce_AdditionalSharedSRVs + Ce_AdditionalCullSRVs) * ce_framesInFlight;// Double or triple buffering

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

#if defined(DX12_TEMPORAL_DRAW_OCCLUSION)
    constexpr uint8_t CE_DX12TEMPORAL_OCCLUSION = 1;
    constexpr uint8_t CE_DX12OCCLUSION = 0;
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