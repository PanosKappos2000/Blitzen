#pragma once
#include "dx12Data.h"
#include "Renderer/Interface/blitRendererInterface.h"
#include "Renderer/Resources/blitRenderingResources.h"
#include "Renderer/Resources/Textures/blitTextures.h"
#include "Core/blitLogger.h"

namespace BlitzenDX12
{
    class Dx12Renderer
    {
    public:

        Dx12Renderer();
        ~Dx12Renderer();

        Dx12Renderer(const Dx12Renderer& dx) = delete;
        Dx12Renderer operator = (const Dx12Renderer& dx) = delete;
    
        // API initialization. Basically loads thing that do not need to depend on CPU resources
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight, HWND hwnd);
    
        // 2nd part of initialization. Initializes handles that require context from the CPU's loaded resources
        uint8_t SetupForRendering(BlitzenEngine::DrawContext& context);

        // Layout transition throwaway function. Some command list incompatibility makes it necessary
        void FinalSetup();
    
        // Function for DDS texture loading
        uint8_t UploadTexture(void* pData, const char* filepath);
    
        // Draws a simple loading screen using a shader that should be valid after Init.
        void DrawWhileWaiting(float deltaTime);
        
        // CPU work while the GPU is busy. Call the fence at the end
        void Update(const BlitzenEngine::DrawContext& context);
    
        // Speaks for itself
        void DrawFrame(BlitzenEngine::DrawContext& context);
    
        // Updates transform data on the cpu side side buffer
        void UpdateObjectTransform(BlitzenEngine::RendererTransformUpdateContext& context);

    public:
        struct FrameTools
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

        struct VarBuffers
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

        struct ConstBuffers
        {
            SSBO vertexBuffer;
            DX12WRAPPER<ID3D12Resource> indexBuffer;
            D3D12_INDEX_BUFFER_VIEW indexBufferView;

            SSBO surfaceBuffer;
            SSBO renderBuffer;
            SSBO lodBuffer;

            SSBO materialBuffer;
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

        DX12WRAPPER<IDXGIFactory6> m_factory;
        DX12WRAPPER<ID3D12Device> m_device;

    private:

        DX12WRAPPER<ID3D12Debug> m_debugController;
        DX12WRAPPER<ID3D12Debug1> m_debugController1;

        DX12WRAPPER<IDXGIAdapter4> m_chosenAdapter;

        DX12WRAPPER<IDXGISwapChain3> m_swapchain;
		UINT m_swapchainWidth;
		UINT m_swapchainHeight;
        // The swapchain will be the color target
        DX12WRAPPER<ID3D12Resource> m_swapchainBackBuffers [ce_framesInFlight];

        DX12WRAPPER<ID3D12Resource> m_depthBuffers[ce_framesInFlight];

    /*
        Descriptors
    */
    private:

        DescriptorContext m_descriptorContext;

        DX12WRAPPER<ID3D12DescriptorHeap> m_srvHeap;
        DX12WRAPPER<ID3D12DescriptorHeap> m_samplerHeap;
        DX12WRAPPER<ID3D12DescriptorHeap> m_rtvHeap;
        DX12WRAPPER<ID3D12DescriptorHeap> m_dsvHeap;

    /*
        Pipelines and Root Signatures
    */
    private:

        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC m_depthTargetDesc{};
        D3D12_RENDER_PASS_RENDER_TARGET_DESC m_renderTargetDesc{};

        DX12WRAPPER<ID3D12RootSignature> m_triangleRootSignature;
        DX12WRAPPER<ID3D12PipelineState> m_trianglePso;

        /* CULLING COMPUTE */
        // All modes
        DX12WRAPPER<ID3D12RootSignature> m_drawCountResetRoot;
        DX12WRAPPER<ID3D12PipelineState> m_drawCountResetPso;
        DX12WRAPPER<ID3D12RootSignature> m_drawCullSignature;
        DX12WRAPPER<ID3D12PipelineState> m_drawCullPso;
        // Draw occlusion mode only
        DX12WRAPPER<ID3D12RootSignature> m_drawOccLateSignature;
        DX12WRAPPER<ID3D12PipelineState> m_drawOccLatePso;
        DX12WRAPPER<ID3D12RootSignature> m_depthPyramidSignature;
        DX12WRAPPER<ID3D12PipelineState> m_depthPyramidPso;
        // Draw Instance cull mode only
        DX12WRAPPER<ID3D12PipelineState> m_drawInstCountResetPso;
        DX12WRAPPER<ID3D12PipelineState> m_drawInstCmdPso;


        DX12WRAPPER<ID3D12CommandSignature> m_opaqueCmdSingature;
        DX12WRAPPER<ID3D12RootSignature> m_opaqueRootSignature;
        // General objects
        DX12WRAPPER<ID3D12PipelineState> m_opaqueGraphicsPso;
        // Transparent
        DX12WRAPPER<ID3D12PipelineState> m_transparentGraphicsPso;

    private:

        DX12WRAPPER<ID3D12CommandQueue> m_commandQueue;
        DX12WRAPPER<ID3D12CommandQueue> m_transferCommandQueue;

        BlitCL::StaticArray<FrameTools, ce_framesInFlight> m_frameTools;
		VarBuffers m_varBuffers[ce_framesInFlight];

        ConstBuffers m_constBuffers;

        Dx12Stats m_stats;

        uint32_t m_currentFrame = 0;

        DX2DTEX m_tex2DList[BlitzenCore::Ce_MaxTextureCount];
        uint32_t m_textureCount{ 0 };

    private:

    };
}