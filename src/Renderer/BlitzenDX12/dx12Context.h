#pragma once 

#if defined(_WIN32)

#include "dx12Data.h"
#include "Renderer/View/blitCamera.h"

namespace BlitzenDX12
{
    struct ReadOnlyResources
    {
        UINT BUFFER_COUNT{ 0 };

        SSBO m_vtxBuffer;

        INDEX_BUFFER m_idxBuffer;

        INDEX_BUFFER m_clusterIdxBuffer;

        SSBO m_surfaceBuffer;

        SSBO m_LODBuffer;

        SSBO m_clusterBuffer;

        SSBO m_matBuffer;

        SSBO m_renderBuffer;

        TEX2D m_drawTextures[BlitzenCore::Ce_MaxTextureCount];
        UINT m_textureCount{ 0 };
    };

    struct ReadWriteResources
    {
        CPU_WRITE_SSBO m_transformBuffer;

        SSBO m_drawCmdBuffer;

        SSBO m_drawCmdCounterBuffer;

        SSBO m_clusterDispatchBuffer;

        SSBO m_clusterVisibilityBuffer;

        SSBO m_clusterGroupDataBuffer;

        SSBO m_drawVisBuffer;

        SSBO m_drawInstBuffer;

        SSBO m_instCounterBuffer;

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

        // Views for descriptors that are used by compute and grahics
        SIZE_T m_sharedViewsOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_sharedViewHandle[ce_framesInFlight];

        // Views for descriptors used by all draw cull shaders
        SIZE_T m_drawCullViewsOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_drawCullViewsHandle[ce_framesInFlight];

        // Views for descriptors used by instanced culling shaders
		SIZE_T m_drawCullInstUAVsOffset[ce_framesInFlight];
		D3D12_GPU_DESCRIPTOR_HANDLE m_drawCullInstUAVsHandle[ce_framesInFlight];

        // View for draw visibility descriptor
		SIZE_T  m_drawVisUAVOffset[ce_framesInFlight];
		D3D12_GPU_DESCRIPTOR_HANDLE m_drawVisUANHandle[ce_framesInFlight];

        // View for depth target descriptor
        SIZE_T m_depthTargetSRVOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_depthTargetSRVHandle[ce_framesInFlight];

        // SRV for hi_z_map
        SIZE_T m_HI_Z_MapSRVOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_HI_Z_MapSRVHandle[ce_framesInFlight];

		// first UAV for all mips of the HI_Z map (the rest are held by the depth pyramid struct)
        SIZE_T m_HI_Z_MapMipsFirstUAVOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_HI_Z_MapMipsFirstUAVHandle[ce_framesInFlight];

        // Views for descriptors only used by opaqueDraw.cs
        SIZE_T m_opaqueDrawViewsExclusiveOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_opaqueDrawViewsExclusiveHandle[ce_framesInFlight];

        // UAV for instance indices descriptor
		SIZE_T m_opaqueDrawInstInstUAVOffset[ce_framesInFlight];
		D3D12_GPU_DESCRIPTOR_HANDLE m_opaqueDrawInstInstUAVHandle[ce_framesInFlight];

        SIZE_T m_clusterDispatchAdditionalUAVsOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE m_clusterDispatchAdditionalUAVsHandle[ce_framesInFlight];

        // View for material descriptor
        SIZE_T m_materialSRVOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE m_materialSRVHandle;

        // Views for texture descriptor array start
        SIZE_T m_texDescriptorsSRVOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE m_texDescriptorsSRVHandle;


        // SAMPLERS
        DX12WRAPPER<ID3D12DescriptorHeap> m_samplerHeap;

        D3D12_GPU_DESCRIPTOR_HANDLE m_samplerHeapHandle;
        SIZE_T m_samplerHeapIncrement;
        SIZE_T m_samplerHeapCurrentOffset{ 0 };

        SIZE_T m_texSmpOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE m_texSmpHandle;


        // RTVs
        DX12WRAPPER<ID3D12DescriptorHeap> m_rtvHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHeapHandle;
        SIZE_T m_rtvHeapIncrement;
        SIZE_T m_rtvHeapOffset{ 0 };

        SIZE_T m_swapchainRtvOffset[ce_framesInFlight];
        D3D12_CPU_DESCRIPTOR_HANDLE m_swapchainRtvHandle[ce_framesInFlight];


        // DSVs
		DX12WRAPPER<ID3D12DescriptorHeap> m_dsvHeap;

		D3D12_CPU_DESCRIPTOR_HANDLE m_dsvHeapHandle;
        SIZE_T m_dsvHeapIncrement;
        SIZE_T m_dsvHeapOffset{ 0 };

        SIZE_T m_depthTargetDsvOffset[ce_framesInFlight];
        D3D12_CPU_DESCRIPTOR_HANDLE m_depthTargetDSVHandle[ce_framesInFlight];
    };

	struct PipelineContext
	{
        // Small compute shader for setting draw count rwssbo to 0
        DX12WRAPPER<ID3D12RootSignature> m_drawCountResetRoot;
        DX12WRAPPER<ID3D12PipelineState> m_drawCountResetPso;

		// Culling compute shader. Performs frustum culling and LOD selection. Creates indirect draw commands
        DX12WRAPPER<ID3D12RootSignature> m_drawCullRoot;
        DX12WRAPPER<ID3D12PipelineState> m_drawCullPso;

        // Small compute shader for instance count rwssbo reset
        // Uses draw cull inst root
		DX12WRAPPER<ID3D12PipelineState> m_drawInstCountResetPso;

        // Culling compute shader. Performs frumtum culling and LOD selection. Sets instance counter
        DX12WRAPPER<ID3D12RootSignature> m_drawCullInstRoot;
		DX12WRAPPER<ID3D12PipelineState> m_drawCullInstPso;

        // Command compute shader. Takes the instance count and sets indirect draw commands for each instance
        // Uses draw cull inst root
		DX12WRAPPER<ID3D12PipelineState> m_drawInstCmdPso;

		// Culling compute shader. Performs frustum culling and LOD selection. Ignores objects that were tagged not visible last frame.
        DX12WRAPPER<ID3D12RootSignature> m_drawOccFirstRoot;
		DX12WRAPPER<ID3D12PipelineState> m_drawOccFirstPso;

        // Resource copy compute shader. Generates HI_Z map for occlusion culling.
        DX12WRAPPER<ID3D12RootSignature> m_HI_Z_MapRoot;
        DX12WRAPPER<ID3D12PipelineState> m_HI_Z_MapPso;
        
        // Culling compute shader. Performs frustum and occlusion culling and LOD selection. 
        // Creates indirect draw commands for objects that were not tagged as visible last frame.
		// Tags objects with their visibility for the next frame.
        DX12WRAPPER<ID3D12RootSignature> m_drawOccLateRoot;
        DX12WRAPPER<ID3D12PipelineState> m_drawOccLatePso;

        // Culling compute shader. Performs frustum and occlusion culling and LOD selection. Uses previous HI_Z map
		// Uses draw occ late root
		DX12WRAPPER<ID3D12PipelineState> m_drawOccTemporalPso;


        // CLUSTER CULLING MODE
        DX12WRAPPER<ID3D12CommandSignature> m_clusterCullCmdSign;
        DX12WRAPPER<ID3D12RootSignature> m_clusterCullRoot;

        DX12WRAPPER<ID3D12PipelineState> m_clusterCullCmdResetPso;

        DX12WRAPPER<ID3D12PipelineState> m_clusterCullDispatchPso;

        DX12WRAPPER<ID3D12PipelineState> m_clusterCullCmdSetPso;

        DX12WRAPPER<ID3D12PipelineState> m_clusterCullPso;

        DX12WRAPPER<ID3D12PipelineState> m_clusterCullBatchCmdPso;

        DX12WRAPPER<ID3D12PipelineState> m_clusterCullBatchPso;


        // Draws triangle with hardcoded vertices in the shader (legacy shader)
        DX12WRAPPER<ID3D12RootSignature> m_triangleRoot;
        DX12WRAPPER<ID3D12PipelineState> m_trianglePso;

        // Vertex and Pixel shaders for opaque objects. Uses execute indirect
        DX12WRAPPER<ID3D12CommandSignature> m_opaqueDrawCmdSign;
        DX12WRAPPER<ID3D12RootSignature> m_opaqueDrawRoot;
        DX12WRAPPER<ID3D12PipelineState> m_opaqueDrawPso;

		// Vertex and Pixel shader for opaque objects with instancing. Uses execute indirect
        DX12WRAPPER<ID3D12CommandSignature> m_opaqueDrawInstCmdSign;
        DX12WRAPPER<ID3D12RootSignature> m_opaqueDrawInstRoot;
        DX12WRAPPER<ID3D12PipelineState> m_opaqueDrawInstPso;

        // Transparent
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
}

#endif 