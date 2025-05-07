#include "dx12Resources.h"
#include "dx12Renderer.h"

namespace BlitzenDX12
{
    uint8_t CreateDescriptorHeaps(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, ID3D12DescriptorHeap** ppSrvHeap, 
        ID3D12DescriptorHeap** ppDsvHeap, ID3D12DescriptorHeap** ppSamplerHeap)
    {
        if (!CheckForDeviceRemoval(device))
        {
            return 0;
        }

        if (!CreateDescriptorHeap(device, ppSrvHeap, Ce_SrvDescriptorCount + BlitzenEngine::ce_maxTextureCount, 
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
        {
            BLIT_ERROR("Failed to create srv descriptor heap");
            return 0;
        } 

        if (!CreateDescriptorHeap(device, ppRtvHeap, ce_framesInFlight, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
        {
            BLIT_ERROR("Failed to create rtv descriptor heap");
            return 0;
        }

        if (!CreateDescriptorHeap(device, ppDsvHeap, ce_framesInFlight, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
        {
            BLIT_ERROR("Failed to create dsv descriptor heap");
            return 0;
        }

        if (!CreateDescriptorHeap(device, ppSamplerHeap, 1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
        {
            BLIT_ERROR("Failed to create sampler descriptor heap");
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
            auto getBackBufferResult = swapchain->GetBuffer(i, IID_PPV_ARGS(backBuffers[i].ReleaseAndGetAddressOf()));
            if (FAILED(getBackBufferResult))
            {
                return LOG_ERROR_MESSAGE_AND_RETURN(getBackBufferResult);
            }
            CreateRenderTargetView(device, Ce_SwapchainFormat, D3D12_RTV_DIMENSION_TEXTURE2D, backBuffers[i].Get(), rtvHeapHandle, descriptorContext.rtvHeapOffset);
        }

        // Success
        return 1;
    }

    uint8_t CreateDepthTargets(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* depthBuffers, D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle,
        Dx12Renderer::DescriptorContext& descriptorContext, uint32_t swapchainWidth, uint32_t swapchainHeight)
    {
        descriptorContext.depthTargetOffset = descriptorContext.dsvHeapOffset;
        for (UINT i = 0; i < ce_framesInFlight; i++)
        {
            D3D12_CLEAR_VALUE clear{};
            clear.Format = Ce_DepthTargetFormat;
            clear.DepthStencil.Depth = Ce_ClearDepth;
            auto resourceRes{ CreateImageResource(device, depthBuffers[i].ReleaseAndGetAddressOf(), swapchainWidth, swapchainHeight, 1,
                Ce_DepthTargetFormat, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear) };
            if (FAILED(resourceRes))
            {
                return LOG_ERROR_MESSAGE_AND_RETURN(resourceRes);
            }

            D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
            viewDesc.Format = Ce_DepthTargetFormat;
            viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            // Creates the view with the right offset
            dsvHeapHandle.ptr += descriptorContext.dsvHeapOffset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            device->CreateDepthStencilView(depthBuffers[i].Get(), &viewDesc, dsvHeapHandle);
            // Increments the offset
            descriptorContext.dsvHeapOffset++;
        }

        // success
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
        SIZE_T& thisOffset, UINT numElements, UINT stride, D3D12_BUFFER_SRV_FLAGS flags /*=D3D12_BUFFER_SRV_FLAG_NONE*/)
    {
        thisOffset = srvOffset;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
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

    void CreateTextureShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle, SIZE_T& srvOffset,
        DXGI_FORMAT format, UINT mipLevels)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = mipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.PlaneSlice = 0;

        handle.ptr += srvOffset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->CreateShaderResourceView(resource, &srvDesc, handle);
        srvOffset++;
    }

    void CreateUnorderedAccessView(ID3D12Device* device, ID3D12Resource* resource, ID3D12Resource* counterResource, D3D12_CPU_DESCRIPTOR_HANDLE handle, 
        SIZE_T& srvOffset, UINT numElements, UINT stride, UINT64 counterOffsetInBytes, D3D12_BUFFER_UAV_FLAGS flags /*=D3D12_BUFFER_UAV_FLAG_NONE*/)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = numElements;
        uavDesc.Buffer.StructureByteStride = stride;
        uavDesc.Buffer.Flags = flags;
        uavDesc.Buffer.CounterOffsetInBytes = counterOffsetInBytes;

        handle.ptr += srvOffset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        device->CreateUnorderedAccessView(resource, counterResource, &uavDesc, handle);

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

    uint8_t CreateImageResource(ID3D12Device* device, ID3D12Resource** ppResource, UINT width, UINT height, UINT mipLevels, DXGI_FORMAT format, 
        D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state, D3D12_CLEAR_VALUE* pClear, D3D12_TEXTURE_LAYOUT layout)
    {
        if (!CheckForDeviceRemoval(device))
        {
            return 0;
        }

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = heapType;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;// probably no need for change
        heapProperties.CreationNodeMask = 0;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment = 0;
        desc.Width = width;  
        desc.Height = height; 
        desc.DepthOrArraySize = 1;
        desc.MipLevels = mipLevels;
        desc.Format = format; 
        desc.SampleDesc.Count = 1;  
        desc.SampleDesc.Quality = 0;
        desc.Layout = layout;
        desc.Flags = flags;

        auto res = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, state, pClear, IID_PPV_ARGS(ppResource));
        if (FAILED(res))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(res);
        }

        // success
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

    void CreateResourceUAVBarrier(D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* pResource, 
        D3D12_RESOURCE_BARRIER_FLAGS flags /*=D3D12_RESOURCE_BARRIER_FLAG_NONE*/)
    {
        barrier = {};

        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = pResource;
        barrier.Flags = flags;
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

    void PlaceFence(UINT64& fenceValue, ID3D12CommandQueue* commandQueue, ID3D12Fence* fence, HANDLE& event)
    {
        const UINT64 fenceV = fenceValue++;
        commandQueue->Signal(fence, fenceV);
        // Waits for the previous frame
        if (fence->GetCompletedValue() < fenceV)
        {
            fence->SetEventOnCompletion(fenceV, event);
            WaitForSingleObject(event, INFINITE);
        }
    }
}