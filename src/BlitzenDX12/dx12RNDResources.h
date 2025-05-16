#pragma once
#include "dx12Resources.h"
#include "dx12Renderer.h"

namespace BlitzenDX12
{
    uint8_t CreateSwapchain(IDXGIFactory6* factory, ID3D12CommandQueue* queue, uint32_t windowWidth, uint32_t windowHeight,
        HWND hwnd, Microsoft::WRL::ComPtr <IDXGISwapChain3>* pSwapchain);

    // Creates backbuffers and render target view needed to present on the swapchain
    uint8_t CreateSwapchainResources(IDXGISwapChain3* swapchain, ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* backBuffers,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle, SIZE_T& rtvHeapOffset);

    uint8_t CreateDepthTargets(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* depthBuffers, D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle,
        SIZE_T& dsvHeapOffset, uint32_t swapchainWidth, uint32_t swapchainHeight);

    void CreateDepthPyramidDescriptors(ID3D12Device* device, Dx12Renderer::VarBuffers* pBuffers, Dx12Renderer::DescriptorContext& context, 
	ID3D12DescriptorHeap* srvHeap, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight);
}