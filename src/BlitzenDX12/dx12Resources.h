#pragma once
#include "dx12Data.h"

namespace BlitzenDX12
{
    uint8_t CreateSwapchainResources(IDXGISwapChain3* swapchain, ID3D12Device* device,
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& rtvHeap, Microsoft::WRL::ComPtr<ID3D12Resource>* backBuffers,
        D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles, D3D12_CPU_DESCRIPTOR_HANDLE& rtvHandle);

    uint8_t CreateDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, D3D12_DESCRIPTOR_HEAP_TYPE type,
        D3D12_DESCRIPTOR_HEAP_FLAGS flags);
}