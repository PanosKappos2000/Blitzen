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
#include "Renderer/Resources/blitShaderShared.h"

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

    /**************************************************************************************************************
    * DESCRIPTOR CONSTANTS SECTION                                                                                *
    ***************************************************************************************************************/
    // SHARED SRVs :
    constexpr UINT CE_GLOBAL_DESCRIPTOR_RANGE_COUNT = 4;
    constexpr UINT CE_GDESC_RENDER_ID = 0;
    constexpr UINT CE_GDESC_TRANSFORM_ID = 1;
    constexpr UINT CE_GDESC_SURFACE_ID = 2;
    constexpr UINT CE_GDESC_VIEW_ID = 3;

    // GLOBAL DESCRIPTORS FOR CULLING:
    constexpr UINT CE_CULL_GLOBAL_RANGE_COUNT = 2;
    constexpr UINT CE_CULL_GLOBAL_LOD_ID = 0;
    constexpr UINT CE_CULL_GLOBAL_BOUNDS_ID = 1;

    // DESCRIPTORS FOR DRAW CULL STATIC OPAQUE :
    constexpr UINT CE_CULL_OS_RANGE_COUNT = 2;
    constexpr UINT CE_CULL_OS_DRAW_CMD_ID = 0;
    constexpr UINT CE_CULL_OS_DRAW_COUNTER_ID = 1;

    // DESCRIPTOR FOR DRAW CULL DYNAMIC OPAQUE
    constexpr UINT CE_CULL_OD_RANGE_COUNT = 4;
    constexpr UINT CE_CULL_OD_DRAW_CMD_ID = 0;
    constexpr UINT CE_CULL_OD_DRAW_COUNTER_ID = 1;
    constexpr UINT CE_CULL_OD_WORLD_VARIABLE_TRANSFORM_ID = 2;
    constexpr UINT CE_CULL_OD_TERRAIN_HEIGHT_ID = 3;

    // DESCRIPTORS FOR DRAW CULL INSTANCES
    constexpr UINT CE_CULL_INST_RANGE_COUNT = 2;
    constexpr UINT CE_CULL_INST_DRAW_CMD_ID = 0;
    constexpr UINT CE_CULL_INST_DRAW_COUNTER_ID = 1;

    // DESCRIPTOR FOR DRAW OCC WITH DOUBLE PASS
    constexpr UINT CE_CULL_OCCFL_RANGE_COUNT = 3;
    constexpr UINT CE_CULL_OCCFL_DRAW_CMD_ID = 0;
    constexpr UINT CE_CULL_OCCFL_DRAW_COUNTER_ID = 1;
    constexpr UINT CE_CULL_OCCFL_DRAW_VISIBILITY_ID = 2;

    // DESCRIPTORS FOR CLUSTER CULLING
    constexpr UINT CE_CULL_CLUSTERS_RANGE_COUNT = 8;
    constexpr UINT CE_CULL_CLUSTERS_CMD_RANGE_ID = 0;
    constexpr UINT CE_CULL_CLUSTERS_CMD_COUNTER_ID = 1;
    constexpr UINT CE_CULL_CLUSTERS_GROUP_DATA_ID = 2;
    constexpr UINT CE_CULL_CLUSTERS_GROUP_COUNTER_ID = 3;
    constexpr UINT CE_CULL_CLUSTERS_VISIBILITY_ID = 4;
    constexpr UINT CE_CULL_CLUSTERS_VTXS_ID = 5;
    constexpr UINT CE_CULL_CLUSTERS_SPHERES_ID = 6;
    constexpr UINT CE_CULL_CLUSTERS_CONES_ID = 7;

    // DESCRIPTORS FOR BROAD PHASE COLLISION
    constexpr UINT GCCollisionSupportRangeCount = 7;
    constexpr UINT GCCollisionSupportGridCellRangeID = 0;
    constexpr UINT GCCollisionSupportColliderIDXsRangeID = 1;
    constexpr UINT GCCollisionSupportGlobalColliderIDXsOffsetRangeID = 2;
    constexpr UINT GCCollisionSupportColliderAMaxRadRangeID = 3;
    constexpr UINT GCCollisionSupportColliderBMinTypeRangeID = 4;
    constexpr UINT GCCollisionSupportTransformColliderAMaxRadRangeID = 5;
    constexpr UINT GCCollisionSupportTransformColliderBMinTypeRangeID = 6;

    // DESCRIPTORS FOR BROAD AND NARROW PHASE COLLISION
    constexpr UINT GCCollisionResolveRangeCount = 10;
    constexpr UINT GCCollisionResolveGridCellRangeID = 0;
    constexpr UINT GCCollisionResolveColliderIDXsRangeID = 1;
    constexpr UINT GCCollisionResolveGlobalColliderIDXsOffsetRangeID = 3;
    constexpr UINT GCCollisionResolveColliderAMaxRadRangeID = 3;
    constexpr UINT GCCollisionResolveColliderBMinTypeRangeID = 4;
    constexpr UINT GCCollisionResolveTransformColliderAMaxRadRangeID = 5;
    constexpr UINT GCCollisionResolveTransformColliderBMinTypeRangeID = 6;
    constexpr UINT GCCollisionResolveNarrowPhaseCMDRangeID = 7;
    constexpr UINT GCCollisionResolveCollisionMessageRangeID = 8;
    constexpr UINT GCCollisionResolveCollisionCounterRangeID = 9;

    // CULLING ROOT PARAMETERS
    constexpr uint32_t CE_CULL_ROOT_PARAMETER_COUNT = 7;
    constexpr UINT GCGlobalDescriptorsRootParameterIDCompute = 0;
    constexpr UINT CE_CULL_ROOT_CULL_GLOBAL_ID = 1;
    constexpr UINT CE_CULL_ROOT_STATIC_TABLE_ID = 2;
    constexpr UINT CE_CULL_ROOT_STATIC_WORK_CONSTANT_ID = 3;
    constexpr UINT CE_CULL_ROOT_HI_Z_MAP_ID = 4;
    constexpr UINT CE_CULL_ROOT_DYNAMIC_TABLE_ID = 5;
    constexpr UINT CE_CULL_ROOT_DYNAMIC_WORK_CONSTANT_ID = 6;

    constexpr UINT CE_CULL_WORK_COUNT_CONSTANT_32_BIT_COUNT = 1;
    constexpr UINT GCBMPRCollisionWorkCountContant32BitCount = 3;
    constexpr UINT GCBMPRCollisionIndirectCellIndex32BitCount = 1;

	// HI Z MAP DESCRIPTORS
    constexpr UINT CE_HI_Z_MAP_ROOT_COUNT = 3;
    constexpr UINT CE_HI_Z_MAP_INPUT_ID = 0;
    constexpr UINT CE_HI_Z_MAP_OUTPUT_ID = 1;
    constexpr UINT CE_HI_Z_MAP_CONSTANT_ID = 2;

    constexpr UINT CE_HI_Z_MAP_CONSTANT_32BIT_COUNT = 5;

    // DESCRIPTORS FOR DRAW VERTEX
    constexpr UINT CE_VERTEX_ODS_RANGE_COUNT = 4;
    constexpr UINT CE_VERTEX_ODS_VTXPOS_ID = 0;
    constexpr UINT CE_VERTEX_ODS_VTXNORMAL_ID = 1;
    constexpr UINT CE_VERTEX_ODS_VTXTANGENT_ID = 2;
    constexpr UINT CE_VERTEX_ODS_VTXTEXCOORD_ID = 3;

    // DESCRIPTORS FOR DRAW PIXEL 
    constexpr UINT CE_PIXEL_ODS_RANGE_COUNT = 1;
    constexpr UINT CE_PIXEL_ODS_MATERIAL_ID = 0;

    constexpr UINT CE_VERTEX_TERRAIN_RANGE_COUNT = 1;
    constexpr UINT CE_VERTEX_TERRAIN_VTXPOS_ID = 0;

    constexpr UINT CE_DRAW_OBJ_ID_32_BIT_COUNT = 1;

    constexpr UINT CE_TEXTURE_DESCRIPTOR_COUNT = BLIT_MAX_WORLD_TEXTURE_RESOURCES;

    // ROOT PARAMETERS FOR GRAPHICS
    constexpr UINT CE_GRAPHICS_ODS_ROOT_COUNT = 8;
    constexpr UINT CE_GRAPHICS_ODS_VTX_TABLE_ID = 0;
    constexpr UINT CE_GRAPHICS_ODS_PS_TABLE_ID = 1;
    constexpr UINT CE_GRAPHICS_ODS_GLOBAL_ID = 2;
    constexpr UINT CE_GRAPHICS_ODS_TEX_ID = 3;
    constexpr UINT CE_GRAPHICS_ODS_TEXSMP_ID = 4;
    constexpr UINT CE_GRAPHICS_ODS_STATIC_OBJIDX_ID = 5;
    constexpr UINT CE_GRAPHICS_ODS_DYNAMIC_OBJIDX_ID = 6;
    constexpr UINT CE_GRAPHICS_TERRAIN_VERTICES_ID = 7;

    // ROOT PARAMETERS FOR BLITZEN LOGO LOADING SCREEN
    constexpr UINT CE_BLITZEN_LOGO_PIPELINE_PARAM_COUNT = 2;
    constexpr UINT CE_BLITZEN_LOGO_TEX_ID = 0;
    constexpr UINT CE_BLITZEN_LOGO_SAMPLER_ID = 1;

    // Descriptors for bounding sphere draw
    constexpr UINT Ce_BoundingSphereRootParameterCount = 3;

	constexpr UINT Ce_BoundingSphereSphereSRVRegister = 0;
	constexpr UINT Ce_BoundingSphereSphereRootParameterID = 0;

	constexpr UINT Ce_BoundingSphereObjectIDConstantRegister = 1;
	constexpr UINT Ce_BoundingSphereObjectIDConstant32BitCount = 1;
    constexpr UINT Ce_BoundingSphereObjectIDRootParameterID = 1;

	constexpr UINT Ce_BoundingSphereViewDataCBVRegister = 0;
    constexpr UINT Ce_BoundingSphereViewDataRootParameterID = 2;


    /* SSBO data copy helpers */
    constexpr UINT Ce_ConstDataSSBOCount = 14;
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
    constexpr UINT CE_TERRAIN_VERTEX_SSBO_STAGING_IDX = 10;
    constexpr UINT CE_TERRAIN_VTX_IDX_SSBO_STAGING_IDX = 11;
    constexpr UINT CE_TERRAIN_HEIGHT_DATA_SSBO_STAGING_IDX = 12;
    constexpr UINT CE_WORLD_VARIABLE_TRANSFORM_STAGING_IDX = 13;
    // optional, when clusters are requested
    constexpr UINT Ce_ClusterVtxsStagingIndex = 13;
    constexpr UINT Ce_ClusterSpheresStagingIndex = 14;
    constexpr UINT Ce_ClusterConesStagingIndex = 15;

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
    class STAGING
    {
    public:
        DX12WRAPPER<ID3D12Resource> m_buffer{ nullptr };
        DATA* m_pMapped{ nullptr };
        SIZE_T m_dataSize{ 0 };
        UINT m_validDataIndex{ 0 };

        inline uint8_t IsValid()
        {
            return m_pMapped != nullptr && m_dataSize != 0;
        }
    };

    template<class DATA>
    class READBACK_BUFFER
    {
    public:
        DX12WRAPPER<ID3D12Resource> m_buffer{ nullptr };
        DATA* m_pMapped{ nullptr };

        inline uint8_t IsValid()
        {
            return m_pMapped != nullptr;
        }
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

    class TEX2D
    {
    public:
        DX12WRAPPER<ID3D12Resource> resource;
        UINT mipLevels{ 0 };
        DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
        D3D12_GPU_DESCRIPTOR_HANDLE view;

        inline uint8_t isValid() { return format != DXGI_FORMAT_UNKNOWN; }
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
        uint32_t objId;
        D3D12_DRAW_INDEXED_ARGUMENTS command;
        uint32_t padding0;
        uint32_t padding1;
    };
    static_assert(sizeof(IndirectDrawCmd) % 16 == 0);

    struct InstancedDrawCmd
    {
        uint32_t instanceOffset;
        uint32_t resourceId;
        D3D12_DRAW_INDEXED_ARGUMENTS command;
        uint32_t padding0;
    };

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