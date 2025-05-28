#pragma once

#if defined(_WIN32)

#include "dx12Context.h"

namespace BlitzenDX12
{
    uint8_t CreateDescriptorHeaps(ID3D12Device* device, DescriptorContext& ctx);

    uint8_t CreateDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, UINT bufferCount, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);

    void CreateConstantBufferView(ID3D12Device* device, DescriptorContext& ctx, ID3D12Resource* pResource, UINT size);

    void CreateBufferShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, DescriptorContext& ctx, UINT numElements, UINT stride, D3D12_BUFFER_SRV_FLAGS flags = D3D12_BUFFER_SRV_FLAG_NONE);

    void CreateTexture2DShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, DescriptorContext& ctx, DXGI_FORMAT format, UINT mipLevels);

    void CreateBufferUnorderedAccessView(ID3D12Device* device, DescriptorContext& ctx, ID3D12Resource* resource, ID3D12Resource* counterResource, 
        UINT numElements, UINT stride, UINT64 counterOffsetInBytes, D3D12_BUFFER_UAV_FLAGS flags = D3D12_BUFFER_UAV_FLAG_NONE);

    void Create2DTextureUnorderedAccessView(ID3D12Device* device, DescriptorContext& ctx, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipSlice);

    void CreateRenderTargetView(ID3D12Device* device, DescriptorContext& ctx, DXGI_FORMAT format, D3D12_RTV_DIMENSION dimension, ID3D12Resource* resource);

    void CreateSampler(ID3D12Device* device, DescriptorContext& ctx, D3D12_TEXTURE_ADDRESS_MODE addressU, D3D12_TEXTURE_ADDRESS_MODE addressV, D3D12_TEXTURE_ADDRESS_MODE addressW,
        FLOAT* pBorderColors, D3D12_FILTER filter, D3D12_COMPARISON_FUNC compFunc = D3D12_COMPARISON_FUNC_NEVER);

    void CreateResourcesTransitionBarrier(D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    void CreateResourceUAVBarrier(D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* pResource, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    uint8_t CreateBuffer(ID3D12Device* device, ID3D12Resource** ppBuffer, UINT64 bufferSize, D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType, 
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    uint8_t CreateImageResource(ID3D12Device* device, ID3D12Resource** ppResource, UINT width, UINT height, UINT mipLevels,DXGI_FORMAT format, 
        D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* pClear, D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN);

    void PlaceFence(UINT64& fenceValue, ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, HANDLE& event);
}

#endif