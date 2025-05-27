#pragma once 

#if defined(_WIN32)

#include "dx12Data.h"
#include "Game/blitCamera.h"

namespace BlitzenDX12
{
    struct ReadOnlyBuffers
    {
        SSBO vertexBuffer;
        DX12WRAPPER<ID3D12Resource> indexBuffer;
        D3D12_INDEX_BUFFER_VIEW indexBufferView;

        SSBO surfaceBuffer;
        SSBO renderBuffer;
        SSBO lodBuffer;

        SSBO materialBuffer;
    };

    struct ReadWriteBuffers
    {
        VarSSBO transformBuffer;

        SSBO indirectDrawBuffer;
        VarSSBO indirectDrawCount;

        SSBO drawVisibilityBuffer;

        SSBO drawInstBuffer;
        SSBO lodInstBuffer;

        CBuffer<BlitzenEngine::CameraViewData> viewDataBuffer;

        DepthPyramid depthPyramid;
    };

    struct DescriptorContext
    {
        /* SRV HEAP */
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;
        SIZE_T srvIncrementSize;
        SIZE_T srvHeapOffset{ 0 };

        SIZE_T sharedSrvOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE sharedSrvHandle[ce_framesInFlight];

        SIZE_T opaqueSrvOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE opaqueSrvHandle[ce_framesInFlight];

        SIZE_T cullSrvOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle[ce_framesInFlight];

        SIZE_T texturesSrvOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle;

        SIZE_T materialSrvOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE materialSrvHandle;

        SIZE_T depthTargetSrvOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE depthTargetSrvHandle[ce_framesInFlight];

        SIZE_T depthPyramidSrvOffset[ce_framesInFlight];
        D3D12_GPU_DESCRIPTOR_HANDLE depthPyramidSrvHandle[ce_framesInFlight];

        SIZE_T depthPyramidMipsSrvOffset[ce_framesInFlight];
        SIZE_T depthPyramidMipsEnd;
        D3D12_GPU_DESCRIPTOR_HANDLE depthPyramidMipsSrvHandle[ce_framesInFlight];


        /* SAMPLER HEAP */
        D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle;
        SIZE_T samplerIncrementSize;
        SIZE_T samplerHeapOffset{ 0 };

        SIZE_T defaultTextureSamplerOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE defaultTextureSamplerHandle;

        SIZE_T depthPyramidSamplerOffset;
        D3D12_GPU_DESCRIPTOR_HANDLE depthPyramidSamplerHandle;


        /* RTV HEAP */
        SIZE_T rtvIncrementSize;
        SIZE_T rtvHeapOffset{ 0 };

        const SIZE_T swapchainRtvOffset{ 0 };
        D3D12_GPU_DESCRIPTOR_HANDLE swapchainRtvHandle;


        /* DSV HEAP */
        SIZE_T dsvIncrementSize;
        SIZE_T dsvHeapOffset{ 0 };

        const SIZE_T depthTargetOffset{ 0 };
        D3D12_GPU_DESCRIPTOR_HANDLE depthTargetDsvHandle;
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
	};

    struct CommandContext
    {
        // Used for graphics and most other operations
        DX12WRAPPER<ID3D12CommandAllocator> mainGraphicsCommandAllocator;
        DX12WRAPPER<ID3D12GraphicsCommandList4> mainGraphicsCommandList;

        // Used for transfer commands in the loading phase, to get better access to all commands
        DX12WRAPPER<ID3D12CommandAllocator> transferCommandAllocator;
        DX12WRAPPER<ID3D12GraphicsCommandList> transferCommandList;

        // Used for transfer commands while drawing, for efficiency
        DX12WRAPPER<ID3D12CommandAllocator> dedicatedTransferAllocator;
        DX12WRAPPER<ID3D12CommandList> dedicatedTransferList;

        DX12WRAPPER<ID3D12Fence> inFlightFence;
        UINT64 inFlightFenceValue;
        HANDLE inFlightFenceEvent;

        DX12WRAPPER<ID3D12Fence> copyFence;
        UINT64 copyFenceValue;
        HANDLE copyFenceEvent;

        uint8_t Init(ID3D12Device* device);
    };
}

#endif 