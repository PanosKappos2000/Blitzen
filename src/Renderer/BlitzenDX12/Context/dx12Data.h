#if defined(_WIN32)

#pragma once
#include <D3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <wrl/client.h>
#include <d3d12sdklayers.h>
#include <comdef.h>
#include "Core/blitzenEngine.h"
#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenDX12
{
    static_assert(sizeof(UINT) == sizeof(uint32_t));
    static_assert(sizeof(SIZE_T) == sizeof(size_t));

    constexpr const char* BLIT_DX12_SYSTEM = BlitzenCore::CE_DX12_SYSTEM_NAME;

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
     
    constexpr UINT Ce_TransformSRVRegister = 1;
    constexpr UINT Ce_TransformSRVRangeID = 1;

    constexpr UINT Ce_RenderSRVRegister = 0;
    constexpr UINT Ce_RenderSRVRangeID = 2;

    constexpr UINT Ce_ViewCBVRegister = 0;
    constexpr UINT Ce_ViewCBVRootID = 3;


    // DESCRIPTORS FOR DRAW CULL :
    constexpr UINT Ce_DrawCullSRVsRangeCount = 4;

    constexpr UINT Ce_DrawCullDrawCmdUAVRegister = 0;
    constexpr UINT Ce_DrawCullDrawCmdUAVRangeID = 0;
    
    constexpr UINT Ce_DrawCullDrawCmdCountUAVRegister = 1;
    constexpr UINT Ce_DrawCullDrawCmdCountUAVRangeID = 1;

    constexpr UINT Ce_DrawCullLODSRVRegister = 7;
    constexpr UINT Ce_DrawCullLODSRVRangeID = 2;

    constexpr UINT Ce_DrawCullBoundingSRVRegister = 11;
    constexpr UINT Ce_DrawCullBoundingRangeID = 3;

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
    constexpr UINT Ce_HI_Z_MapMipLvlContant32BitCount = 5;

	// Root param descriptor grouping
    constexpr UINT Ce_HI_Z_MapRootParameterCount = 3;

    constexpr UINT Ce_HI_Z_MapUAVRootID = 0;
    constexpr UINT Ce_HI_Z_MapSRVRootID = 1;
    constexpr UINT Ce_HI_Z_MapMipLvlConstantRootID = 2;

    // DESCRIPTOR FOR DRAW OCC LATE:
    // additional SRVs
    constexpr UINT Ce_DrawOccLateDrawVisUAVRegister = 5;

    // HI_Z MAP For occlusion
    constexpr UINT Ce_DrawOccLateHI_Z_MapSRVRegister = 3;

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

    // DESCRIPTOR FOR CLUSTER CULL:
    constexpr UINT Ce_ClusterDispatchAdditionalViewsRangeCount = 6;

    // Cluster dispatch command buffer
    constexpr UINT Ce_ClusterCullCmdUAVRegister = 5;
    constexpr UINT Ce_ClusterCullCmdUAVRangeID = 0;

    // Cluster dispatch counter buffer
    constexpr UINT Ce_ClusterCullCounterUAVRegister = 6;
    constexpr UINT Ce_ClusterCullCounterUAVRangeID = 1;

    // Cluster group data buffer
    constexpr UINT Ce_ClusterCullGroupDataUAVRegister = 7;
    constexpr UINT Ce_ClusterCullGroupDataUAVRangerID = 2;

    // Cluster attributes(3)
    constexpr UINT Ce_ClusterCullClusterVtxsSRVRegister = 8;
    constexpr UINT Ce_ClusterCullClusterVtxsSRVRangeID = 3;

    constexpr UINT Ce_ClusterCullClusterSpheresSRVRegister = 9;
    constexpr UINT Ce_ClusterCullClusterSpheresSRVRangeID = 4;

    constexpr UINT Ce_ClusterCullClusterConesSRVRegister = 10;
    constexpr UINT Ce_ClusterCullClusterConesSRVRangeID = 5;

    // root param descriptor grouping
    constexpr UINT Ce_ClusterCullRootParameterCount = 5;

    constexpr UINT Ce_ClusterCullExclusiveSRVsRootID = 0;
    constexpr UINT Ce_ClusterCullSharedSRVsRootID = 1;
    constexpr UINT Ce_ClusterCullDrawCountRootID = 2;
    constexpr UINT Ce_ClusterCullAdditionalViewsRootID = 3;
    constexpr UINT Ce_ClusterCullHI_Z_MapSrvRootID = 4;

    // DESCRIPTORS FOR OPAQUE DRAW :
    // exclusive range specific
    constexpr UINT Ce_OpaqueDrawExclusiveSRVsRangeCount = 4;

    constexpr UINT Ce_OpaqueDrawVtxPosSRVRegister = 3;
    constexpr UINT Ce_OpaqueDrawVtxPosSRVRangeID = 0;

    constexpr UINT Ce_OpaqueDrawVtxNormalSRVRegister = 4;
    constexpr UINT Ce_OpaqueDrawVtxNormalSRVRangeID = 1;

    constexpr UINT Ce_OpaqueDrawVtxTangentSRVRegister = 5;
    constexpr UINT Ce_OpaqueDrawVtxTangentSRVRangeID = 2;

    constexpr UINT Ce_OpaqueDrawVtxTexCoordSRVRegister = 6;
    constexpr UINT Ce_OpaqueDrawVtxTexCoordSRVRangeID = 3;

    // exclusive ps range 
    constexpr UINT Ce_OpaqueDrawPSExclusiveSRVsRangeCount = 1;

    constexpr UINT Ce_OpaqueDrawPSMaterialSRVRegister = 7;
    constexpr UINT Ce_OpaqueDrawPSMaterialSRVRangeID = 0;

    // texture sampler
    constexpr UINT Ce_OpaqueDrawTexSMPRegister = 0;

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

    // Descriptors for bounding sphere draw
    constexpr UINT Ce_BoundingSphereRootParameterCount = 3;

	constexpr UINT Ce_BoundingSphereSphereSRVRegister = 0;
	constexpr UINT Ce_BoundingSphereSphereRootParameterID = 0;

	constexpr UINT Ce_BoundingSphereObjectIDConstantRegister = 1;
	constexpr UINT Ce_BoundingSphereObjectIDConstant32BitCount = 1;
    constexpr UINT Ce_BoundingSphereObjectIDRootParameterID = 1;

	constexpr UINT Ce_BoundingSphereViewDataCBVRegister = 0;
    constexpr UINT Ce_BoundingSphereViewDataRootParameterID = 2;

    // Root param descriptor grouping 
    constexpr UINT Ce_OpaqueDrawInstRootParameterCount = 7;
    constexpr UINT Ce_OpaqueDrawInstExclusiveSRVsRootID = 0;
    constexpr UINT Ce_OpaqueDrawInstSharedSRVsRootID = 1;
    constexpr UINT Ce_OpaqueDrawInstObjIDRootID = 2;
    constexpr UINT Ce_OpaqueDrawInstTexSMPRootID = 3;
    constexpr UINT Ce_OpaqueDrawInstMatSRVRootID = 4;
    constexpr UINT Ce_OpaqueDrawInstTexSRVRootID = 5;
    constexpr UINT Ce_OpaqueDrawInstInstSRVRootID = 6;


    // VIEW HEAP DESCRIPTOR COUNT
    constexpr UINT Ce_MaterialSRVDescriptorCount = 1;
    constexpr UINT Ce_DrawVisUavDescriptorCount = 1;
	constexpr UINT Ce_DepthTargetSRVDescriptorCount = 1;
	constexpr UINT Ce_HI_Z_MapSRVDescriptorCount = 1;
    constexpr UINT Ce_ClusterDispatchUAVsCount = 4;
    constexpr UINT Ce_ClusterCullClustersSRVCount = 1;

    // SAMPLER HEAP DESCRIPTOR COUNT
    constexpr UINT Ce_TexSmpDescriptorCount = 1;


    /* SSBO data copy helpers */
    constexpr UINT Ce_ConstDataSSBOCount = 9;
    constexpr UINT Ce_VtxPosStagingBufferIndex = 0;
    constexpr UINT Ce_IndexStagingBufferIndex = 1;
    constexpr UINT Ce_SurfaceStagingBufferIndex = 2;
    constexpr UINT Ce_RenderStagingBufferIndex = 3;
    constexpr UINT Ce_LodStagingIndex = 4;
    constexpr UINT Ce_MaterialStagingIndex = 5;
    constexpr UINT Ce_VtxTangentsStagingBufferIndex = 6;
    constexpr UINT Ce_VtxNrmStagingBufferIndex = 7;
    constexpr UINT Ce_VtxTexCoordStagingBufferIndex = 8;
    constexpr UINT Ce_BoundingSphereBoundingIndex = 9;
    // optional, when clusters are requested
    constexpr UINT Ce_ClusterVtxsStagingIndex = 10;
    constexpr UINT Ce_ClusterSpheresStagingIndex = 11;
    constexpr UINT Ce_ClusterConesStagingIndex = 12;
    
    constexpr UINT Ce_VarBuffersCount = 3 * ce_framesInFlight;

    constexpr UINT Ce_VarSSBODataCount = 1;

    constexpr UINT Ce_TransformStagingBufferIndex = 0;

    // Buffer size used for texture staging buffer
    constexpr SIZE_T Ce_TextureDataStagingSize = 128 * 1024 * 1024;

// OCCLUSION MODES
#if defined(BLITZEN_CLUSTER_CULLING) || defined(BLITZEN_DRAW_TEMPORAL_OCCLUSION) || defined(DX12_OCCLUSION_DRAW_CULL)
    constexpr uint8_t CE_DX12_BUILD_HI_Z_MAP = 1;
#else
    constexpr uint8_t CE_DX12_BUILD_HI_Z_MAP = 0;
#endif


    struct Dx12Stats
    {
        uint8_t bDiscreteGPU = 0;

        uint8_t bResourceManagement = 0;
    };

    template<class DX12TYPE>
	using DX12WRAPPER = Microsoft::WRL::ComPtr<DX12TYPE>;

    template<typename DATA>
    struct CBUFFER
    {
        DX12WRAPPER<ID3D12Resource> buffer;

        DATA* pData{ nullptr };
    };

    struct SSBO
    {
        DX12WRAPPER<ID3D12Resource> buffer{ nullptr };
    };

    template<class DATA>
    struct STAGING
    {
        DX12WRAPPER<ID3D12Resource> m_buffer{ nullptr };
        DATA* m_pMapped{ nullptr };
        SIZE_T m_dataSize{ 0 };
    };

    template<class DATA>
    struct CPU_WRITE_SSBO
    {
        SSBO m_ssbo{};

        STAGING<DATA> m_dynamicDataStaging{};

        SIZE_T heapOffset{};
    };

    struct INDEX_BUFFER
    {
        DX12WRAPPER<ID3D12Resource> m_buffer;
        D3D12_INDEX_BUFFER_VIEW m_view;
    };

    struct TEX2D
    {
        DX12WRAPPER<ID3D12Resource> resource;
        UINT mipLevels{ 0 };
        DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
        D3D12_GPU_DESCRIPTOR_HANDLE view;
    };

    struct HI_Z_MAP
    {
        DX12WRAPPER<ID3D12Resource> pyramid;
        uint32_t width{ 0 };
        uint32_t height{ 0 };

        UINT mipCount{ 0 };
        D3D12_GPU_DESCRIPTOR_HANDLE mips[Ce_DepthPyramidMaxMips];
    };

    struct FENCE
    {
        DX12WRAPPER<ID3D12Fence> m_dx12Handle;
        UINT64 m_value;
        HANDLE m_event;
    };

    struct SWAPCHAIN
    {
        DX12WRAPPER<IDXGISwapChain3> swapchain;
        UINT width;
        UINT height;
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
    constexpr uint32_t Ce_IndirectDrawCmdBufferSize = 500'000;

    struct ClusterGroupData
    {
        uint32_t objId;
        uint32_t clusterOffset;
        uint32_t clusterCount;
        uint32_t visibleAny;
    };
    static_assert(sizeof(ClusterGroupData) % 16 == 0);
    constexpr uint32_t Ce_ClusterGroupDataBufferSize = 1'000'000;

    struct ClusterDispatchCmd
    {
        D3D12_DISPATCH_ARGUMENTS command;// 3 32 bit integers

        uint32_t padding0;
    };
    static_assert(sizeof(ClusterDispatchCmd) % 16 == 0);


    // Useful helper to check for device removal before calling a function that uses it
    uint8_t CheckForDeviceRemoval(ID3D12Device* device);

    // If a dx12 functcion fails, it can calls this to log the result and return 0
    uint8_t LOG_ERROR_MESSAGE_AND_RETURN(HRESULT res);
}

#endif