#pragma once

#if defined(_WIN32)

#include "dx12Context.h"

namespace BlitzenDX12
{
    uint8_t CreateDescriptorHeaps(ID3D12Device* device, DescriptorContext& ctx);

    uint8_t CreateDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, UINT bufferCount, 
        D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);

    void CreateBufferShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, DescriptorContext& ctx, UINT numElements, UINT stride, D3D12_BUFFER_SRV_FLAGS flags = D3D12_BUFFER_SRV_FLAG_NONE);

    void CreateTexture2DShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, DescriptorContext& ctx, DXGI_FORMAT format, UINT mipLevels);

    void CreateBufferUnorderedAccessView(ID3D12Device* device, DescriptorContext& ctx, ID3D12Resource* resource, ID3D12Resource* counterResource, 
        UINT numElements, UINT stride, UINT64 counterOffsetInBytes, D3D12_BUFFER_UAV_FLAGS flags = D3D12_BUFFER_UAV_FLAG_NONE);

    void Create2DTextureUnorderedAccessView(ID3D12Device* device, DescriptorContext& ctx, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipSlice);

    void CreateRenderTargetView(ID3D12Device* device, DescriptorContext& ctx, DXGI_FORMAT format, D3D12_RTV_DIMENSION dimension, ID3D12Resource* resource);

    void CreateSampler(ID3D12Device* device, DescriptorContext& ctx, D3D12_TEXTURE_ADDRESS_MODE addressU, D3D12_TEXTURE_ADDRESS_MODE addressV, D3D12_TEXTURE_ADDRESS_MODE addressW,
        FLOAT* pBorderColors, D3D12_FILTER filter, D3D12_COMPARISON_FUNC compFunc = D3D12_COMPARISON_FUNC_NEVER);

    void CreateResourcesTransitionBarrier(D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    void CreateResourceUAVBarrier(D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* pResource, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);

    uint8_t CreateBuffer(ID3D12Device* device, ID3D12Resource** ppBuffer, UINT64 bufferSize,
        D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    UINT64 CreateIndexBuffer(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>& indexBuffer, DX12WRAPPER<ID3D12Resource>& stagingBuffer,
        size_t elementCount, void* pData, D3D12_INDEX_BUFFER_VIEW& ibv);

    uint8_t CreateImageResource(ID3D12Device* device, ID3D12Resource** ppResource, UINT width, UINT height, UINT mipLevels,DXGI_FORMAT format, 
        D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* pClear, 
        D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN);

    uint8_t CreateDepthPyramidResource(ID3D12Device* device, DepthPyramid& depthPyramid, uint32_t width, uint32_t height);

    void PlaceFence(UINT64& fenceValue, ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, HANDLE& event);


    template<typename DATA>
    UINT64 CreateSSBO(ID3D12Device* device, SSBO& ssbo, DX12WRAPPER<ID3D12Resource>& stagingBuffer, size_t elementCount, DATA* data, 
        D3D12_RESOURCE_FLAGS ssboFlags = D3D12_RESOURCE_FLAG_NONE)
    {
        // SSBO (GPU side buffer)
		if (!CreateBuffer(device, ssbo.buffer.ReleaseAndGetAddressOf(), sizeof(DATA) * elementCount, D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_DEFAULT, ssboFlags))
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
    UINT64 CreateVarSSBO(ID3D12Device* device, VarSSBO& ssbo, DX12WRAPPER<ID3D12Resource>& tempStaging, 
        size_t elementCount, DATA* data, UINT persistentStagingBufferSize)
    {
        // SSBO (GPU side buffer)
        if (!CreateBuffer(device, ssbo.buffer.ReleaseAndGetAddressOf(), sizeof(DATA) * elementCount, D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_DEFAULT))
        {
            return 0;
        }

        // Staging buffer (CPU side buffer)
        if (!CreateBuffer(device, tempStaging.ReleaseAndGetAddressOf(), sizeof(DATA) * elementCount, D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_UPLOAD))
        {
            return 0;
        }

        // Staging buffer holds the data for the SSBO
        void* pMappedData{ nullptr };
        auto mappingRes{ tempStaging->Map(0, nullptr, &pMappedData) };
        if (FAILED(mappingRes))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }
        BlitzenCore::BlitMemCopy(pMappedData, data, sizeof(DATA) * elementCount);

        if (!CreateBuffer(device, ssbo.staging.ReleaseAndGetAddressOf(), sizeof(DATA) * persistentStagingBufferSize,
            D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
        {
            return 0;
        }

        mappingRes = ssbo.staging->Map(0, nullptr, &ssbo.pData);
        if (FAILED(mappingRes))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }
        ssbo.dataCopySize = sizeof(DATA) * persistentStagingBufferSize;
        BlitzenCore::BlitMemCopy(ssbo.pData, data, ssbo.dataCopySize);

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

#endif