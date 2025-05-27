#pragma once

#if defined(_WIN32)

#include "dx12Resources.h"
#include "dx12Renderer.h"

namespace BlitzenDX12
{
    uint8_t CreateSwapchain(IDXGIFactory6* factory, ID3D12CommandQueue* queue, uint32_t windowWidth, uint32_t windowHeight,
        HWND hwnd, Microsoft::WRL::ComPtr <IDXGISwapChain3>* pSwapchain);

    // Creates backbuffers and render target view needed to present on the swapchain
    uint8_t CreateSwapchainResources(IDXGISwapChain3* swapchain, ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* backBuffers, DescriptorContext& ctx);

    uint8_t CreateDepthTargets(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* depthBuffers, DescriptorContext& ctx, uint32_t swapchainWidth, uint32_t swapchainHeight);

    void CreateDepthPyramidDescriptors(ID3D12Device* device, Dx12Renderer::VarBuffers* pBuffers, DescriptorContext& context, 
        DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight);

    void CopyDepthPyramidToSwapchain(ID3D12GraphicsCommandList4* commandList, ID3D12Resource* swapchainBackBuffer, ID3D12Resource* depthPyramid,
        UINT depthPyramidWidth, UINT depthPyramidHeight, ID3D12DescriptorHeap* descriptorHeap, ID3D12CommandQueue* queue, IDXGISwapChain3* swapchain, 
        uint32_t pyramidMip, uint32_t swapchainWidht, uint32_t swapchainHeight);
}

#endif