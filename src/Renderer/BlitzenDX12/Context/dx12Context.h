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

        SSBO m_surfaceBuffer{};
        SSBO m_LODBuffer{};
        SSBO m_matBuffer{};

        SSBO m_renderBuffer{};
        SSBO m_boundingSpheres{};

        SSBO m_instancedRenders;

        TEX2D m_drawTextures[BlitzenCore::Ce_MaxTextureCount];
        UINT m_textureCount{ 0 };

        STAGING<BlitzenEngine::CPU_TRANSFORM> CPU_MOVING_OBJECT_BUFFER{};
    };

    struct ReadWriteResources
    {
        SSBO m_transformBuffer;

        SSBO m_staticDrawCmdBuffer;
        SSBO m_staticDrawCmdCounter;

        SSBO m_movementBuffer;
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

    struct DescriptorContext
    {
        DX12WRAPPER<ID3D12DescriptorHeap> m_viewHeap;

        // Base view heap
        D3D12_GPU_DESCRIPTOR_HANDLE m_viewHeapHandle;
        SIZE_T m_viewHeapIncrement;
        SIZE_T m_viewHeapCurrentOffset{ 0 };

        SIZE_T m_globalTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_globalTableHandle[ce_framesInFlight];

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

        SIZE_T m_cullOCCDPTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_cullOCCDPTableHandle[ce_framesInFlight];

		SIZE_T  m_HI_Z_MAP_cullOffset[ce_framesInFlight];
		D3D12_GPU_DESCRIPTOR_HANDLE m_HI_Z_MAP_cullHandle[ce_framesInFlight];

        SIZE_T m_depthTargetOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_depthTargetHandle[ce_framesInFlight];

        SIZE_T m_HI_Z_MAP_mipOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_HI_Z_MAP_mipHandle[ce_framesInFlight];
        SIZE_T m_HI_Z_MAP_firstUAVOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_HI_Z_MapMipsFirstUAVHandle[ce_framesInFlight];

        SIZE_T m_vertexODSTableOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_vertexODSTableHandle[ce_framesInFlight];

		SIZE_T m_pixelODSTableOffset;
		D3D12_GPU_DESCRIPTOR_HANDLE m_pixelODSTableHandle;

        SIZE_T m_texturesTableOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE m_texturesTableHandle;

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

        D3D12_RENDER_PASS_RENDER_TARGET_DESC m_renderTargetPassDesc[ce_framesInFlight]{};
        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC m_depthTargetPassDesc[ce_framesInFlight]{};
	};

    struct CmdContext
    {
        DX12WRAPPER<ID3D12CommandAllocator> m_graphicsCmdAlloc;
        DX12WRAPPER<ID3D12GraphicsCommandList4> m_graphicsCmdList;

        DX12WRAPPER<ID3D12CommandAllocator> m_copyCmdAlloc;
        DX12WRAPPER<ID3D12GraphicsCommandList> m_copyCmdList;

        FENCE m_frameFence;

        FENCE m_copyFence;

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
        STAGING<BlitzenEngine::CPU_TRANSFORM> m_cpuTransformStaging;
    };
}

#endif 