#pragma once
#include "dx12Data.h"
#include "Renderer/blitRenderingResources.h"
#include "Renderer/blitDDSTextures.h"
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
        uint8_t SetupForRendering(BlitzenEngine::RenderingResources* pResources, float& pyramidWidth, float& pyramidHeight);

        // Layout transition throwaway function. Some command list incompatibility makes it necessary
        void FinalSetup();
    
        // Function for DDS texture loading
        uint8_t UploadTexture(void* pData, const char* filepath);
    
        // Draws a simple loading screen using a shader that should be valid after Init.
        void DrawWhileWaiting();
        
        // CPU work while the GPU is busy. Call the fence at the end
        void Update(const BlitzenEngine::DrawContext& context);
    
        // Speaks for itself
        void DrawFrame(BlitzenEngine::DrawContext& context);
    
        // Updates transform data on the cpu side side buffer
        void UpdateObjectTransform(uint32_t trId, BlitzenEngine::MeshTransform& newTr);

        inline static Dx12Renderer* GetRendererInstance() { return s_pThis; }

    public:
        struct FrameTools
        {
            // Used for graphics and most other operations
            DX12WRAPPER<ID3D12CommandAllocator> mainGraphicsCommandAllocator;
            DX12WRAPPER<ID3D12GraphicsCommandList> mainGraphicsCommandList;

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

            CBuffer<BlitzenEngine::CameraViewData> viewDataBuffer;
        };

        struct ConstBuffers
        {
            SSBO vertexBuffer;
            SSBO surfaceBuffer;
            SSBO renderBuffer;
            SSBO lodBuffer;

            DX12WRAPPER<ID3D12Resource> indexBuffer;
            D3D12_INDEX_BUFFER_VIEW indexBufferView;
        };

        struct DescriptorContext
        {
            D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;
            SIZE_T srvIncrementSize;
            SIZE_T srvHeapOffset{ 0 };// Current offset into the srv heap for adding views
            SIZE_T sharedSrvOffset[ce_framesInFlight];// Offset of srvHeap for shared descriptor table
            SIZE_T opaqueSrvOffset[ce_framesInFlight];// Offset of srvHeap for opaque pipeline descriptor table
            SIZE_T cullSrvOffset[ce_framesInFlight];// Offset of srvHeap for cull pipeline descriptor table

            SIZE_T rtvHeapOffset{ 0 };
            SIZE_T swapchainRtvOffset;

            SIZE_T dsvHeapOffset{ 0 };
            SIZE_T depthTargetOffset;
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
        DX12WRAPPER<ID3D12DescriptorHeap> m_rtvHeap;
        DX12WRAPPER<ID3D12DescriptorHeap> m_dsvHeap;

        D3D12_DESCRIPTOR_RANGE m_opaqueDescriptorRanges[Ce_OpaqueSrvRangeCount]{};

    /*
        Pipelines and Root Signatures
    */
    private:

        DX12WRAPPER<ID3D12RootSignature> m_triangleRootSignature;
        DX12WRAPPER<ID3D12PipelineState> m_trianglePso;

        DX12WRAPPER<ID3D12PipelineState> m_drawCountResetPso;
        DX12WRAPPER<ID3D12RootSignature> m_drawCountResetRoot;

        DX12WRAPPER<ID3D12PipelineState> m_drawCull1Pso;
        DX12WRAPPER<ID3D12RootSignature> m_drawCullSignature;

        DX12WRAPPER<ID3D12CommandSignature> m_opaqueCmdSingature;
        DX12WRAPPER<ID3D12RootSignature> m_opaqueRootSignature;
        DX12WRAPPER<ID3D12PipelineState> m_opaqueGraphicsPso;

    private:

        DX12WRAPPER<ID3D12CommandQueue> m_commandQueue;
        DX12WRAPPER<ID3D12CommandQueue> m_transferCommandQueue;

        BlitCL::StaticArray<FrameTools, ce_framesInFlight> m_frameTools;
		VarBuffers m_varBuffers[ce_framesInFlight];

        ConstBuffers m_constBuffers;

        Dx12Stats m_stats;

        uint32_t m_currentFrame = 0;

        DX12WRAPPER<ID3D12Resource> m_textureResources[BlitzenEngine::ce_maxTextureCount];
        uint32_t m_textureCount{ 0 };

    private:

		static Dx12Renderer* s_pThis;
    };

    uint8_t CreateSwapchain(IDXGIFactory6* factory, ID3D12CommandQueue* queue, uint32_t windowWidth, uint32_t windowHeight, 
        HWND hwnd, Microsoft::WRL::ComPtr <IDXGISwapChain3>* pSwapchain);

    // Creates backbuffers and render target view needed to present on the swapchain
    uint8_t CreateSwapchainResources(IDXGISwapChain3* swapchain, ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* backBuffers,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle, Dx12Renderer::DescriptorContext& descriptorContext);

    uint8_t CreateDepthTargets(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* depthBuffers, D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle,
        Dx12Renderer::DescriptorContext& descriptorContext, uint32_t swapchainWidth, uint32_t swapchainHeight);
}