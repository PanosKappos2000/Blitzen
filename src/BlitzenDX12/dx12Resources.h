#pragma once
#include "dx12Data.h"

namespace BlitzenDX12
{
    uint8_t CreateDescriptorHeaps(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, ID3D12DescriptorHeap** ppSrvHeap, 
        ID3D12DescriptorHeap** ppDsvHeap);

    uint8_t CreateDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, UINT bufferCount, 
        D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);

    void CreateBufferShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle, SIZE_T& srvOffset,
		SIZE_T& thisOffset, UINT numElements, UINT stride, D3D12_BUFFER_SRV_FLAGS flags = D3D12_BUFFER_SRV_FLAG_NONE);

    void CreateUnorderedAccessView(ID3D12Device* device, ID3D12Resource* resource, ID3D12Resource* counterResource, D3D12_CPU_DESCRIPTOR_HANDLE handle,
        SIZE_T& srvOffset, UINT numElements, UINT stride, UINT64 counterOffsetInBytes, D3D12_BUFFER_UAV_FLAGS flags = D3D12_BUFFER_UAV_FLAG_NONE);

    void CreateRenderTargetView(ID3D12Device* device, DXGI_FORMAT format, D3D12_RTV_DIMENSION dimension, ID3D12Resource* resource,
        D3D12_CPU_DESCRIPTOR_HANDLE handle, SIZE_T& rtvOffset);

    void CreateResourcesTransitionBarrier(D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    uint8_t CreateBuffer(ID3D12Device* device, ID3D12Resource** ppBuffer, UINT64 bufferSize,
        D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    UINT64 CreateIndexBuffer(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>& indexBuffer, DX12WRAPPER<ID3D12Resource>& stagingBuffer,
        size_t elementCount, void* pData, D3D12_INDEX_BUFFER_VIEW& ibv);

    uint8_t CreateImageResource(ID3D12Device* device, ID3D12Resource** ppResource, UINT width, UINT height, UINT mipLevels,
        DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* pClear);


    template<typename DATA>
    UINT64 CreateSSBO(ID3D12Device* device, SSBO& ssbo, DX12WRAPPER<ID3D12Resource>& stagingBuffer, size_t elementCount, DATA* data)
    {
        // SSBO (GPU side buffer)
		if (!CreateBuffer(device, ssbo.buffer.ReleaseAndGetAddressOf(), sizeof(DATA) * elementCount, D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_DEFAULT))
		{
            return 0;
		}

		// Staging buffer (CPU side buffer)
        if (!CreateBuffer(device, stagingBuffer.ReleaseAndGetAddressOf(), sizeof(DATA) * elementCount, D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_UPLOAD))
        {
            return 0;
        }
        // Staging buffer holds the data for the SSBO
		void* pMappedData{ nullptr };
        auto mappingRes{ stagingBuffer->Map(0, nullptr, &pMappedData) };
        if (FAILED(mappingRes))
        {
			return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }
		BlitzenCore::BlitMemCopy(pMappedData, data, sizeof(DATA) * elementCount);

        // Success
        return sizeof(DATA) * elementCount;
    }

    template<typename DATA>
    UINT64 CreateVarSSBO(ID3D12Device* device, VarSSBO& ssbo, size_t elementCount,DATA* data)
    {
        // SSBO (GPU side buffer)
        if (!CreateBuffer(device, ssbo.buffer.ReleaseAndGetAddressOf(), sizeof(DATA) * elementCount, D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_DEFAULT))
        {
            return 0;
        }

        // Staging buffer (CPU side buffer)
        if (!CreateBuffer(device, ssbo.staging.ReleaseAndGetAddressOf(), sizeof(DATA) * elementCount, D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_UPLOAD))
        {
            return 0;
        }
        // Staging buffer holds the data for the SSBO
        auto mappingRes{ ssbo.staging->Map(0, nullptr, &ssbo.pData) };
        if (FAILED(mappingRes))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }
        BlitzenCore::BlitMemCopy(ssbo.pData, data, sizeof(DATA) * elementCount);

        // Success
        return sizeof(DATA) * elementCount;
    }

    template<typename DATA>
    uint8_t CreateCBuffer(ID3D12Device* device, CBuffer<DATA>& cBuffer)
    {
        if (!CreateBuffer(device, cBuffer.buffer.ReleaseAndGetAddressOf(), sizeof(DATA), D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_UPLOAD))
        {
            return 0;
        }

        auto mappingRes{ cBuffer.buffer->Map(0, nullptr, &(reinterpret_cast<void*>(cBuffer.pData))) };
        if (FAILED(mappingRes))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }

        // Success
        return 1;
    }
}