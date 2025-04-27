#include "dx12Resources.h"
#include "dx12Renderer.h"

namespace BlitzenDX12
{
    uint8_t CreateSwapchainResources(IDXGISwapChain3* swapchain, ID3D12Device* device, 
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& rtvHeap, Microsoft::WRL::ComPtr<ID3D12Resource>* backBuffers,
        D3D12_CPU_DESCRIPTOR_HANDLE* rtvHandles, D3D12_CPU_DESCRIPTOR_HANDLE& rtvHandle)
    {
        if (!CheckForDeviceRemoval(device))
        {
            return 0;
        }

        // Creates the RTV heap first
		if (!CreateDescriptorHeap(device, rtvHeap.ReleaseAndGetAddressOf(), ce_framesInFlight,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
		{
            return 0;
		}

        // Creates back buffers and render target views
		rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < ce_framesInFlight; i++)
        {
            auto geBackBufferResult = swapchain->GetBuffer(i, IID_PPV_ARGS(backBuffers[i].ReleaseAndGetAddressOf()));
            if (FAILED(geBackBufferResult))
            {
                return LOG_ERROR_MESSAGE_AND_RETURN(geBackBufferResult);
            }

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = Ce_SwapchainFormat;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            // Assign the handle to the specific RTV in the heap
            rtvHandles[i] = rtvHandle;
            device->CreateRenderTargetView(backBuffers[i].Get(), &rtvDesc, rtvHandles[i]);

            // Move the handle to the next slot in the heap
            rtvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        // Success
        return 1;
    }

    uint8_t CreateDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, UINT bufferCount,
        D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = bufferCount;
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

    void CreateBufferShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle,
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
		device->CreateShaderResourceView(resource, &srvDesc, handle);
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
}