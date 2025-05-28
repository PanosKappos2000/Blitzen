#pragma once

#if defined(_WIN32)

#include "dx12Context.h"
#include "Renderer/Interface/blitRendererInterface.h"
#include "Renderer/Resources/blitRenderingResources.h"
#include "Renderer/Resources/Textures/blitTextures.h"
#include "Core/DbLog/blitLogger.h"
#include "Platform/blitPlatformContext.h"

namespace BlitzenDX12
{
    class Dx12Renderer
    {
    public:

        Dx12Renderer() = default;
        Dx12Renderer(const Dx12Renderer& dx) = delete;
        Dx12Renderer operator = (const Dx12Renderer& dx) = delete;

        ~Dx12Renderer();
    
        uint8_t Init(uint32_t windowWidth, uint32_t windowHeight, BlitzenPlatform::PlatformContext* pContext);
    
        uint8_t SetupForRendering(BlitzenEngine::DrawContext& context);

        void FinalSetup();
    
        uint8_t UploadTexture(const char* filepath);
    
        void DrawWhileWaiting(float deltaTime);
        
        void Update(const BlitzenEngine::DrawContext& context);
    
        void DrawFrame(BlitzenEngine::DrawContext& context);
    
        void UpdateObjectTransform(uint32_t transformId, BlitzenEngine::MeshTransform* pTransform);

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

        DX12WRAPPER<IDXGIFactory6> m_factory;

        DX12WRAPPER<ID3D12Device> m_device;

        Dx12Stats m_stats;

    private:

        DX12WRAPPER<ID3D12Debug> m_debugController;
        DX12WRAPPER<ID3D12Debug1> m_debugController1;

        DX12WRAPPER<IDXGIAdapter4> m_chosenAdapter;

        DX12WRAPPER<IDXGISwapChain3> m_swapchain;
		UINT m_swapchainWidth;
		UINT m_swapchainHeight;
        
        DX12WRAPPER<ID3D12Resource> m_swapchainBackBuffers [ce_framesInFlight];

        DX12WRAPPER<ID3D12Resource> m_depthBuffers[ce_framesInFlight];

        uint32_t m_currentFrame{ 0 };

        BlitCL::StaticArray<FrameTools, ce_framesInFlight> m_frameTools;

        DX12WRAPPER<ID3D12CommandQueue> m_commandQueue;

        DX12WRAPPER<ID3D12CommandQueue> m_transferCommandQueue;

        ReadWriteResources m_rwResources[ce_framesInFlight];
        
        ReadOnlyResources m_roResources;

        DescriptorContext m_descriptorContext;

        PipelineContext m_pipelineContext;

    };
}

#endif