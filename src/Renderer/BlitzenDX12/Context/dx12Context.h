#if defined(_WIN32)
#pragma once 
#include "dx12Data.h"
#include "Renderer/View/blitCamera.h"
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenDX12
{
    struct ReadOnlyResources
    {
        UINT BUFFER_COUNT{ 0 };

        SSBO m_vtxPosBuffer{};
        SSBO m_vtxTexCoordBuffer{};
        SSBO m_vtxNrmBuffer{};
        SSBO m_vtxTangentBuffer{};

        INDEX_BUFFER m_idxBuffer{};

        SSBO m_clusterVtxsBuffer{};
        SSBO m_clusterSpheresBuffer{};
        SSBO m_clusterConesBuffer{};

        INDEX_BUFFER m_clusterIdxBuffer{};

        SSBO m_terrainVtxBuffer{};
        INDEX_BUFFER m_terrainIdxBuffer{};
        SSBO m_terrainHeightBuffer{};

        SSBO m_surfaceBuffer{};
        SSBO m_LODBuffer{};
        SSBO m_matBuffer{};

        SSBO m_renderBuffer{};
        SSBO m_boundingSpheres{};

        SSBO m_instancedRenders;

        TEX2D m_drawTextures[BlitzenCore::Ce_MaxTextureCount];
        UINT m_textureCount{ 0 };
    };

    struct ReadWriteResources
    {
        SSBO m_transformBuffer;

        SSBO m_staticDrawCmdBuffer;
        SSBO m_staticDrawCmdCounter;

        SSBO m_dynamicDrawCmdBuffer;
        SSBO m_dynamicDrawCmdCounter;

        SSBO m_clusterGroupDataBuffer;
        SSBO m_clusterGroupCounter;

        SSBO m_clusterVisibilityBuffer;
        SSBO m_clusterDrawCmdBuffer;
        SSBO m_clusterDrawCounter;

        SSBO m_drawVisBuffer;

        SSBO m_instanceDrawCmdBuffer;

        CBUFFER<BlitzenEngine::CameraViewData> m_viewBuffer;

        HI_Z_MAP m_HI_Z;
    };

    struct CPU_LOGIC_BUFFERS
    {
        SSBO GPUSSBOWorldVariableTransform{}; // BlitzenEngine::WVTransform -> float3 + float3 + uint32 ID + uint32 flag

        SSBO UAVGridCellWorldVariableOffsets{}; // BlitzenEngine::GridCellOffsets -> 2 uint32 | Collision with BMPR(Any mode) only. Otherwise not created
        SSBO UAVGridCellStaticOffsets{}; // BlitzenEngine::GridCellOffsets -> uint32 | Narrow Phase Collision with BMPR only. Otherwise not created
        SSBO UAVGlobalColliderIDXOffset{}; // 1 uint32 | Collision with BMPR(Any mode) only. Otherwise not created
        SSBO UAVColliderIndices{}; // uint32 | Collision with BMPR(Any mode) only. Size depends on narrow phase or No narrow phase
        SSBO UAVColliderAMaxRad{}; // BlitzenEngine::ColliderAMaxRad -> float4 | Collision with BMPR only. Size depends on narrow phase or No narrow phase
        SSBO UAVColliderBMinType{}; // BlitzenEngine::ColliderBMinType -> float4 | Collision with BMPR only. Size depends on narrow phase or No narrow phase

        SSBO UAVNarrowPhaseIndirectCommands{};
        SSBO UAVCollisionMessageCounter{};
        SSBO UAVCollisionMessages{};

        // World Variable Transform copy to GPU and World Variable transform readback on CPU.
        STAGING<BlitzenEngine::WVTransform> CPURWWorldVariableTransforms{};
        READBACK_BUFFER<BlitzenEngine::WVTransform> CPURDBWorldVariableTransforms{};

        // These readbacks are only relevant when Broad Phase with BMPR is active without Narrow Phase
        // If narrow phase is driven by the BMPR, then all this data does not need to be read
        READBACK_BUFFER<BlitzenEngine::GridCellOffsets> RDBGridCellWorldVariableOffsets{};
        READBACK_BUFFER<uint32_t> RDBWorldVariableColliderIndices{};
        READBACK_BUFFER<BlitzenEngine::ColliderAMaxRad> RDBColliderFloatAMaxRad{};
        READBACK_BUFFER<BlitzenEngine::ColliderBMinType> RDBColliderFloatBMinType{};
        READBACK_BUFFER<uint32_t> RDBCollisionMessageCounter{};
        READBACK_BUFFER<BlitzenEngine::CollisionMessage> RDBCollisionMessage{};
    };

    struct DescriptorContext
    {
        DX12WRAPPER<ID3D12DescriptorHeap> m_viewHeap;

        // Base view heap
        D3D12_GPU_DESCRIPTOR_HANDLE m_viewHeapHandle;
        SIZE_T m_viewHeapIncrement;
        SIZE_T m_viewHeapCurrentOffset{ 0 };

        SIZE_T mGlobalDescriptorsTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE mGlobalDescriptorsTableHandle[ce_framesInFlight];

        SIZE_T m_cullGlobalTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_cullGlobalTableHandle[ce_framesInFlight];

        SIZE_T m_cullOSTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_cullOSTableHandle[ce_framesInFlight];

        SIZE_T m_cullODTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_cullODTableHandle[ce_framesInFlight];

		SIZE_T m_cullInstTableOffset[ce_framesInFlight];
		D3D12_GPU_DESCRIPTOR_HANDLE m_cullInstTableHandle[ce_framesInFlight];

        SIZE_T m_cullClusterTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_cullClusterTableHandle[ce_framesInFlight];

        SIZE_T mCollisionSupportTableOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE mCollisionSupportTableHandle;

        SIZE_T m_cullOCCDPTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_cullOCCDPTableHandle[ce_framesInFlight];

		SIZE_T  m_HI_Z_MAP_cullOffset[ce_framesInFlight];
		D3D12_GPU_DESCRIPTOR_HANDLE m_HI_Z_MAP_cullHandle[ce_framesInFlight];

        SIZE_T m_depthTargetOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_depthTargetHandle[ce_framesInFlight];

        SIZE_T m_HI_Z_MAP_mipOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_HI_Z_MAP_mipHandle[ce_framesInFlight];

        SIZE_T m_vertexODSTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_vertexODSTableHandle[ce_framesInFlight];

		SIZE_T m_pixelODSTableOffset;
		D3D12_GPU_DESCRIPTOR_HANDLE m_pixelODSTableHandle;

        SIZE_T m_texturesTableOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE m_texturesTableHandle;

        SIZE_T m_terrainVertexTableOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE m_terrainVertexTableHandle;

        SIZE_T m_blitzenLogoTextureTableOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE m_blitzenLogoTextureTableHandle;

        DX12WRAPPER<ID3D12DescriptorHeap> m_samplerHeap;

        D3D12_GPU_DESCRIPTOR_HANDLE m_samplerHeapHandle;
        SIZE_T m_samplerHeapIncrement;
        SIZE_T m_samplerHeapCurrentOffset{ 0 };

        SIZE_T m_texSmpOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE m_texSmpHandle;

        DX12WRAPPER<ID3D12DescriptorHeap> m_rtvHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHeapHandle;
        SIZE_T m_rtvHeapIncrement;
        SIZE_T m_rtvHeapOffset{ 0 };

        SIZE_T m_swapchainRtvOffset[ce_framesInFlight];
        D3D12_CPU_DESCRIPTOR_HANDLE m_swapchainRtvHandle[ce_framesInFlight];

		DX12WRAPPER<ID3D12DescriptorHeap> m_dsvHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHeapHandle;
        SIZE_T m_dsvHeapIncrement;
        SIZE_T m_dsvHeapOffset{ 0 };

        SIZE_T m_depthTargetDsvOffset[ce_framesInFlight];
        D3D12_CPU_DESCRIPTOR_HANDLE m_depthTargetDSVHandle[ce_framesInFlight];

		D3D12_GPU_DESCRIPTOR_HANDLE m_viewDataHandle[ce_framesInFlight];
		D3D12_GPU_DESCRIPTOR_HANDLE m_boundSpheresBufferHandle[ce_framesInFlight];
    };

	struct PipelineContext
	{
        DX12WRAPPER<ID3D12RootSignature> m_cullRoot;
        UINT m_cullInstTableRootID;
        UINT m_cullInstWorkRootID;
        UINT m_clusterCullTableRootID;
        UINT m_clusterCullWorkRootID;

        DX12WRAPPER<ID3D12PipelineState> m_staticCullPso;
        DX12WRAPPER<ID3D12PipelineState> m_drawOccTemporalPso;
        DX12WRAPPER<ID3D12PipelineState> m_opaqueStaticCountResetPso;

        DX12WRAPPER<ID3D12PipelineState> m_dynamicCullPso;
        DX12WRAPPER<ID3D12PipelineState> m_opaqueDynamicCountResetPso;

		DX12WRAPPER<ID3D12PipelineState> m_drawCullInstPso;
        DX12WRAPPER<ID3D12PipelineState> m_instanceCountResetPso;

		DX12WRAPPER<ID3D12PipelineState> m_drawOccFirstPso;
        DX12WRAPPER<ID3D12PipelineState> m_drawOccLatePso;

        DX12WRAPPER<ID3D12PipelineState> m_clusterCullDispatchPso;
        DX12WRAPPER<ID3D12CommandSignature> m_clusterCullCmdSign;
        DX12WRAPPER<ID3D12PipelineState> m_clusterCullCmdResetPso;
        DX12WRAPPER<ID3D12PipelineState> m_clusterCullPso;
        DX12WRAPPER<ID3D12PipelineState> m_clusterCullBatchCmdPso;
        DX12WRAPPER<ID3D12PipelineState> m_clusterCullBatchPso;

        DX12WRAPPER<ID3D12RootSignature> m_HI_Z_MapRoot;
        DX12WRAPPER<ID3D12PipelineState> m_HI_Z_MapPso;

        // Collision help. BMPR drives collision. First five are broad phase
        DX12WRAPPER<ID3D12PipelineState> MCellsColliderCountResetPso;
        DX12WRAPPER<ID3D12PipelineState> MCellsColliderCountPso;
        DX12WRAPPER<ID3D12PipelineState> MCellsColliderOffsetPso;
        DX12WRAPPER<ID3D12PipelineState> MColliderIDXsPso;
        DX12WRAPPER<ID3D12PipelineState> MColliderTransformPso;
        // Narrow phase collision
        DX12WRAPPER<ID3D12PipelineState> MNarrowPhaseCollisionPso;

        DX12WRAPPER<ID3D12RootSignature> m_triangleRoot;
        DX12WRAPPER<ID3D12PipelineState> m_trianglePso;

		DX12WRAPPER<ID3D12RootSignature> m_boundingSphereRoot;
		DX12WRAPPER<ID3D12PipelineState> m_boundingSpherePso;

        DX12WRAPPER<ID3D12RootSignature> m_graphicsRoot;
        UINT m_clusterObjidxContantRootID;

        DX12WRAPPER<ID3D12CommandSignature> m_staticDrawCmdSignature;
        DX12WRAPPER<ID3D12PipelineState> m_staticDrawPso;
        
        DX12WRAPPER<ID3D12CommandSignature> m_dynamicDrawCmdSignature;
        DX12WRAPPER<ID3D12PipelineState> m_dynamicDrawPso;

        DX12WRAPPER<ID3D12CommandSignature> m_drawInstCmdSignature;
        DX12WRAPPER<ID3D12PipelineState> m_drawInstPso;

        DX12WRAPPER<ID3D12CommandSignature> m_transparentDrawCmdSignature;
        DX12WRAPPER<ID3D12PipelineState> m_transparentDrawPso;

        DX12WRAPPER<ID3D12CommandSignature> m_terrainDrawCmdSignature;
        DX12WRAPPER<ID3D12PipelineState> m_terrainDrawPso;

        D3D12_RENDER_PASS_RENDER_TARGET_DESC m_renderTargetPassDesc[ce_framesInFlight]{};
        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC m_depthTargetPassDesc[ce_framesInFlight]{};

        DX12WRAPPER<ID3D12PipelineState> m_blitzenLogoPipelineState;
        DX12WRAPPER<ID3D12RootSignature> m_blitzenLogoRoot;
	};

    struct CmdContext
    {
        DX12WRAPPER<ID3D12CommandAllocator> m_graphicsCmdAlloc;
        DX12WRAPPER<ID3D12GraphicsCommandList4> m_graphicsCmdList;
        FENCE m_frameFence;

        DX12WRAPPER<ID3D12CommandAllocator> m_copyCmdAlloc;
        DX12WRAPPER<ID3D12GraphicsCommandList> m_copyCmdList;
        FENCE m_copyFence;

		DX12WRAPPER<ID3D12CommandAllocator> m_computeCmdAlloc;
        DX12WRAPPER<ID3D12GraphicsCommandList> m_computeCmdList;
        FENCE m_computeFence;

        uint8_t Init(ID3D12Device* device);
    };

    struct LoadingContextMesh
    {
        STAGING<BlitzenEngine::VtxPos> m_vtxPosStaging;
        STAGING<BlitzenEngine::VtxNormals> m_vtxNrmStaging;
        STAGING<BlitzenEngine::VtxTangents> m_vtxTngStaging;
        STAGING<BlitzenEngine::VtxTexCoords> m_vtxTexCoordStaging;
        STAGING<uint32_t> m_vtxIdxStaging;
        STAGING<BlitzenEngine::ClusterVertices> m_clusterVtxStaging;
        STAGING<BlitzenEngine::ClusterSphere> m_clusterSpheresStaging;
        STAGING<BlitzenEngine::ClusterCone> m_clusterConesStaging;
        STAGING<uint32_t> m_clusterIdxStaging;
        STAGING<BlitzenEngine::PrimitiveSurface> m_meshPrimStaging;
        STAGING<BlitzenEngine::LodData> m_lodDataStaging;
    };

    struct LoadingContextRenderObjects
    {
        STAGING<BlitzenEngine::RenderObject> m_renderStaging;
        STAGING<BlitzenEngine::RenderObject> m_dynamicRenderStaging;
        STAGING<BlitzenEngine::MeshTransform> m_transformStaging;
        STAGING<BlitzenEngine::WVTransform> m_cpuTransformStaging;
        STAGING<BlitzenEngine::BoundingSphere> m_boundingSpheresStaging;
    };
}

#endif 