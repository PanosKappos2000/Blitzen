#include "dx12Resources.h"
#include "dx12Renderer.h"

namespace BlitzenDX12
{
    uint8_t CreateDescriptorHeaps(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, ID3D12DescriptorHeap** ppSrvHeap)
    {
        if (!CheckForDeviceRemoval(device))
        {
            return 0;
        }

        if (!CreateDescriptorHeap(device, ppSrvHeap, Ce_SrvDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
        {
            BLIT_ERROR("Failed to create srv descriptor heap");
            return 0;
        }

        if (!CreateDescriptorHeap(device, ppRtvHeap, ce_framesInFlight, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
        {
            BLIT_ERROR("Failed to create rtv descriptor heap");
            return 0;
        }

        // success
        return 1;
    }

    uint8_t CreateSwapchainResources(IDXGISwapChain3* swapchain, ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* backBuffers,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle, Dx12Renderer::DescriptorContext& descriptorContext)
    {
        descriptorContext.swapchainRtvOffset = descriptorContext.rtvHeapOffset;
        for (UINT i = 0; i < ce_framesInFlight; i++)
        {
            auto geBackBufferResult = swapchain->GetBuffer(i, IID_PPV_ARGS(backBuffers[i].ReleaseAndGetAddressOf()));
            if (FAILED(geBackBufferResult))
            {
                return LOG_ERROR_MESSAGE_AND_RETURN(geBackBufferResult);
            }
            CreateRenderTargetView(device, Ce_SwapchainFormat, D3D12_RTV_DIMENSION_TEXTURE2D, backBuffers[i].Get(), rtvHeapHandle, descriptorContext.rtvHeapOffset);
        }

        // Success
        return 1;
    }

    uint8_t CreateDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, UINT descriptorCount,
        D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = descriptorCount;
        rtvHeapDesc.Type = type;
        rtvHeapDesc.Flags = flags;

        auto descriptorHeapResult = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(ppRtvHeap));
        if (FAILED(descriptorHeapResult))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(descriptorHeapResult);
        }

        // Success
        return 1;
    }

    void CreateRenderTargetView(ID3D12Device* device, DXGI_FORMAT format, D3D12_RTV_DIMENSION dimension, ID3D12Resource* resource, 
        D3D12_CPU_DESCRIPTOR_HANDLE handle, SIZE_T& rtvOffset)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = Ce_SwapchainFormat;
        rtvDesc.ViewDimension = dimension;

        handle.ptr += rtvOffset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device->CreateRenderTargetView(resource, &rtvDesc, handle);

        // Increase the srv offset
        rtvOffset++;
    }

    void CreateBufferShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle, SIZE_T& srvOffset,
        D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, UINT numElements, UINT stride, D3D12_BUFFER_SRV_FLAGS flags /*=D3D12_BUFFER_SRV_FLAG_NONE*/)
    {
		srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;
		srvDesc.Buffer.StructureByteStride = stride;
		srvDesc.Buffer.Flags = flags;

        // Find the offset of the srv registers for this heap
        handle.ptr += srvOffset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		device->CreateShaderResourceView(resource, &srvDesc, handle);

        // Increase srv offset
        srvOffset++;
    }

    uint8_t CreateBuffer(ID3D12Device* device, ID3D12Resource** ppBuffer, UINT64 bufferSize,
        D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags /*=D3D12_RESOURCE_FLAG_NONE*/)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = heapType;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;// probably no need for change
        heapProperties.CreationNodeMask = 0;

        // Description
        D3D12_RESOURCE_DESC ssboBufferDesc = {};
        ssboBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        ssboBufferDesc.Alignment = 0;
        ssboBufferDesc.Width = bufferSize;
        ssboBufferDesc.Flags = flags;

        // Common for Buffers
        ssboBufferDesc.Height = 1;
        ssboBufferDesc.DepthOrArraySize = 1;
        ssboBufferDesc.MipLevels = 1;
        ssboBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        ssboBufferDesc.SampleDesc.Count = 1;
        ssboBufferDesc.SampleDesc.Quality = 0;
        ssboBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        auto resourceRes = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &ssboBufferDesc,
            initialState, nullptr, IID_PPV_ARGS(ppBuffer));
        if (FAILED(resourceRes))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(resourceRes);
        }

        //Success
        return 1;
    }

    void CreateResourcesTransitionBarrier(D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, 
        UINT subresource/*=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/,
        D3D12_RESOURCE_BARRIER_FLAGS flags/*=D3D12_RESOURCE_BARRIER_FLAG_NONE*/)
    {
        barrier = {};

        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = flags;
        barrier.Transition.pResource = pResource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = subresource;

    }

    UINT64 CreateIndexBuffer(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>& indexBuffer, DX12WRAPPER<ID3D12Resource>& stagingBuffer,
        size_t elementCount, void* pData, D3D12_INDEX_BUFFER_VIEW& ibv)
    {
        // SSBO (GPU side buffer)
        if (!CreateBuffer(device, indexBuffer.ReleaseAndGetAddressOf(), sizeof(uint32_t) * elementCount, 
            D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT))
        {
            return 0;
        }

        // Staging buffer (CPU side buffer)
        if (!CreateBuffer(device, stagingBuffer.ReleaseAndGetAddressOf(), sizeof(uint32_t) * elementCount, 
            D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
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
        BlitzenCore::BlitMemCopy(pMappedData, pData, sizeof(uint32_t) * elementCount);

        ibv = {};
        ibv.BufferLocation = indexBuffer->GetGPUVirtualAddress();
        ibv.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * elementCount);
        ibv.Format = DXGI_FORMAT_R32_UINT;

        // Success
        return sizeof(uint32_t) * elementCount;
    }
}