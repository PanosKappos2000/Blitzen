#pragma once

#if defined(_WIN32)
#include "dx12Context.h"
#include "Renderer/Interface/blitRendererInterface.h"
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
    
        uint8_t UploadTexture(const char* filepath);
    
        void DrawWhileWaiting(float deltaTime);

        void UpdateTransforms(BlitzenEngine::MeshTransform* pTransform, uint32_t transformCount, BlitzenEngine::MeshTransform* transforms);

        void DrawFrame(BlitzenEngine::DrawContext& context);

        void Present(UINT placeHolderCount = 1);

    public:

        Dx12Stats m_stats;

        uint32_t m_currentFrame{ 0 };

        UINT m_swapchainIDX;

        DX12WRAPPER<IDXGIFactory6> m_factory;

        DX12WRAPPER<ID3D12Device> m_device;

        DX12WRAPPER<ID3D12Debug> m_debugController;
        DX12WRAPPER<ID3D12Debug1> m_debugController1;

        DX12WRAPPER<IDXGIAdapter4> m_chosenAdapter;

        DX12WRAPPER<IDXGISwapChain3> m_swapchain;
        UINT m_swapchainWidth;
        UINT m_swapchainHeight;

        CmdContext m_cmdContext[ce_framesInFlight];

        DX12WRAPPER<ID3D12CommandQueue> m_commandQueue;

        DX12WRAPPER<ID3D12CommandQueue> m_transferCommandQueue;
    
        DX12WRAPPER<ID3D12Resource> m_swapchainBackBuffers [ce_framesInFlight];

        DX12WRAPPER<ID3D12Resource> m_depthBuffers[ce_framesInFlight];

        ReadWriteResources m_rwResources[ce_framesInFlight];
        
        ReadOnlyResources m_roResources;

        DescriptorContext m_descriptorContext;

        PipelineContext m_pipelineContext;

    };
}

#endif