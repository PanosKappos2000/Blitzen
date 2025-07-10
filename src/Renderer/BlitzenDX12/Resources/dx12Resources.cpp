#if defined(_WIN32)
#include "dx12Resources.h"
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenDX12
{
    uint8_t CreateDescriptorHeaps(ID3D12Device* device, DescriptorContext& ctx)
    {
        if (!CheckForDeviceRemoval(device))
        {
            return 0;
        }

        constexpr UINT BlitzenLogoTextureDescriptorCount = 1;
        constexpr UINT HIERACHICAL_Z_BUFFER_OutputDescriptorCount = 1;
        constexpr UINT HIERACHICAL_Z_BUFFER_InputDescriptorCount = 1;

        UINT srvHeapDescriptorCount = BlitzenLogoTextureDescriptorCount;// For Blitzen Logo texture
        srvHeapDescriptorCount += CE_GLOBAL_DESCRIPTOR_RANGE_COUNT * ce_framesInFlight;
        srvHeapDescriptorCount += CE_CULL_GLOBAL_RANGE_COUNT * ce_framesInFlight;
        srvHeapDescriptorCount += CE_VERTEX_ODS_RANGE_COUNT * ce_framesInFlight;
        srvHeapDescriptorCount += CE_PIXEL_ODS_RANGE_COUNT;
        srvHeapDescriptorCount += CE_TEXTURE_DESCRIPTOR_COUNT;
        srvHeapDescriptorCount += CE_VERTEX_TERRAIN_RANGE_COUNT;
        srvHeapDescriptorCount += CE_CULL_OS_RANGE_COUNT * ce_framesInFlight;
        srvHeapDescriptorCount += CE_CULL_OD_RANGE_COUNT * ce_framesInFlight;

        if (BlitzenCore::Ce_InstanceCulling)
        {
			srvHeapDescriptorCount += CE_CULL_INST_RANGE_COUNT * ce_framesInFlight;// This include the descriptor used by the graphics pipeline in instanced mode
        }

		if (BlitzenCore::CE_OCCLUSION_DOUBLE_PASS)
		{
            srvHeapDescriptorCount += ce_framesInFlight;
		}

        if (BlitzenCore::Ce_Build_HI_Z)
        {
            srvHeapDescriptorCount += (Ce_DepthPyramidMaxMips + HIERACHICAL_Z_BUFFER_OutputDescriptorCount + HIERACHICAL_Z_BUFFER_InputDescriptorCount) * ce_framesInFlight;
        }

        if (BlitzenCore::Ce_BuildClusters)
        {
            srvHeapDescriptorCount += CE_CULL_CLUSTERS_RANGE_COUNT * ce_framesInFlight;
        }

        if (!CreateDescriptorHeap(device, ctx.m_viewHeap.ReleaseAndGetAddressOf(), srvHeapDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
        {
            BLIT_ERROR("Failed to create srv descriptor heap");
            return 0;
        } 
        ctx.m_viewHeapHandle = ctx.m_viewHeap->GetGPUDescriptorHandleForHeapStart();
		ctx.m_viewHeapIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        if (!CreateDescriptorHeap(device, ctx.m_rtvHeap.ReleaseAndGetAddressOf(), ce_framesInFlight, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
        {
            BLIT_ERROR("Failed to create rtv descriptor heap");
            return 0;
        }
		ctx.m_rtvHeapHandle = ctx.m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
		ctx.m_rtvHeapIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        if (!CreateDescriptorHeap(device, ctx.m_dsvHeap.ReleaseAndGetAddressOf(), ce_framesInFlight, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
        {
            BLIT_ERROR("Failed to create dsv descriptor heap");
            return 0;
        }
		ctx.m_dsvHeapHandle = ctx.m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
		ctx.m_dsvHeapIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        if (!CreateDescriptorHeap(device, ctx.m_samplerHeap.ReleaseAndGetAddressOf(), 1, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
        {
            BLIT_ERROR("Failed to create sampler descriptor heap");
            return 0;
        }
		ctx.m_samplerHeapHandle = ctx.m_samplerHeap->GetGPUDescriptorHandleForHeapStart();
		ctx.m_samplerHeapIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        // success
        return 1;
    }

    uint8_t CreateDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, UINT descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = descriptorCount;
        rtvHeapDesc.Type = type;
        rtvHeapDesc.Flags = flags;

        HRESULT descriptorHeapResult = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(ppRtvHeap));
        if (FAILED(descriptorHeapResult))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(descriptorHeapResult);
        }

        // Success
        return 1;
    }

    void CreateRenderTargetView(ID3D12Device* device, DescriptorContext& ctx, DXGI_FORMAT format, D3D12_RTV_DIMENSION dimension, ID3D12Resource* resource)
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = Ce_SwapchainFormat;
        rtvDesc.ViewDimension = dimension;
        
        // Handle for heap start
		auto handle = ctx.m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += ctx.m_rtvHeapOffset * ctx.m_rtvHeapIncrement;

        // Creates view
        device->CreateRenderTargetView(resource, &rtvDesc, handle);

        // Increments the srv offset
        ctx.m_rtvHeapOffset++;
    }

    void CreateConstantBufferView(ID3D12Device* device, DescriptorContext& ctx, ID3D12Resource* pResource, UINT size)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};

        cbvDesc.BufferLocation = pResource->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = size;

        auto descriptorHandle = ctx.m_viewHeap->GetCPUDescriptorHandleForHeapStart();
        descriptorHandle.ptr += ctx.m_viewHeapCurrentOffset * ctx.m_viewHeapIncrement;

        device->CreateConstantBufferView(&cbvDesc, descriptorHandle);

        ctx.m_viewHeapCurrentOffset++;
    }

    void CreateBufferShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, DescriptorContext& ctx, UINT numElements, UINT stride, D3D12_BUFFER_SRV_FLAGS flags)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = numElements;
		srvDesc.Buffer.StructureByteStride = stride;
		srvDesc.Buffer.Flags = flags;

		// handle for heap start
		auto handle{ ctx.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
        handle.ptr += ctx.m_viewHeapCurrentOffset * ctx.m_viewHeapIncrement;

        // Creates view
		device->CreateShaderResourceView(resource, &srvDesc, handle);

        // Increments offset
        ctx.m_viewHeapCurrentOffset++;
    }

    void CreateTexture2DShaderResourceView(ID3D12Device* device, ID3D12Resource* resource, DescriptorContext& ctx, DXGI_FORMAT format, UINT mipLevels)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = mipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.PlaneSlice = 0;

        // handle for heap start
        auto handle{ ctx.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
        handle.ptr += ctx.m_viewHeapCurrentOffset * ctx.m_viewHeapIncrement;

        // Creates view
        device->CreateShaderResourceView(resource, &srvDesc, handle);

        // Increments offset
        ctx.m_viewHeapCurrentOffset++;
    }

    void CreateBufferUnorderedAccessView(ID3D12Device* device, DescriptorContext& ctx, ID3D12Resource* resource, ID3D12Resource* counterResource,
        UINT numElements, UINT stride, UINT64 counterOffsetInBytes, D3D12_BUFFER_UAV_FLAGS flags)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = numElements;
        uavDesc.Buffer.StructureByteStride = stride;
        uavDesc.Buffer.Flags = flags;
        uavDesc.Buffer.CounterOffsetInBytes = counterOffsetInBytes;

		// Handle for heap start
		auto handle{ ctx.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
        handle.ptr += ctx.m_viewHeapCurrentOffset * ctx.m_viewHeapIncrement;
        device->CreateUnorderedAccessView(resource, counterResource, &uavDesc, handle);

        ctx.m_viewHeapCurrentOffset++;
    }

    void Create2DTextureUnorderedAccessView(ID3D12Device* device, DescriptorContext& ctx, ID3D12Resource* resource, DXGI_FORMAT format, UINT mipSlice)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = mipSlice;
        uavDesc.Texture2D.PlaneSlice = 0;

		// Handle for heap start
		auto handle{ ctx.m_viewHeap->GetCPUDescriptorHandleForHeapStart() };
        handle.ptr += ctx.m_viewHeapCurrentOffset * ctx.m_viewHeapIncrement;

        // Creates view
        device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, handle);

        // Increments offset 
        ctx.m_viewHeapCurrentOffset++;
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

        HRESULT resourceRes = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &ssboBufferDesc, initialState, nullptr, IID_PPV_ARGS(ppBuffer));
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

        HRESULT res = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, state, pClear, IID_PPV_ARGS(ppResource));
        if (FAILED(res))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(res);
        }

        // success
        return 1;
    }

    void CreateResourcesTransitionBarrier(D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter, 
        UINT subresource/*=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/, D3D12_RESOURCE_BARRIER_FLAGS flags/*=D3D12_RESOURCE_BARRIER_FLAG_NONE*/)
    {
        barrier = {}; 

        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = flags;
        barrier.Transition.pResource = pResource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = subresource;
    }

    void CreateResourceUAVBarrier(D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* pResource, D3D12_RESOURCE_BARRIER_FLAGS flags /*=D3D12_RESOURCE_BARRIER_FLAG_NONE*/)
    {
        barrier = {};

        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = pResource;
        barrier.Flags = flags;
    }

    void CreateSampler(ID3D12Device* device, DescriptorContext& ctx, D3D12_TEXTURE_ADDRESS_MODE addressU, D3D12_TEXTURE_ADDRESS_MODE addressV, D3D12_TEXTURE_ADDRESS_MODE addressW, 
        FLOAT* pBorderColors, D3D12_FILTER filter, D3D12_COMPARISON_FUNC compFunc/*D3D12_COMPARISON_FUNC_NEVER*/)
    {
        D3D12_SAMPLER_DESC desc{};
        desc.AddressU = addressU;
        desc.AddressV = addressV;
        desc.AddressW = addressW;

        if (pBorderColors)
        {
            desc.BorderColor[0] = pBorderColors[0];
            desc.BorderColor[1] = pBorderColors[1];
            desc.BorderColor[2] = pBorderColors[2];
            desc.BorderColor[3] = pBorderColors[3];
        }

        desc.ComparisonFunc = compFunc;
        desc.Filter = filter;
        desc.MaxAnisotropy = filter & D3D12_FILTER_ANISOTROPIC ? 4 : 1;
        desc.MinLOD = 0.f;
        desc.MaxLOD = D3D12_FLOAT32_MAX;
        desc.MipLODBias = 0.f;

        // Gets current handle for heap start
		auto handle{ ctx.m_samplerHeap->GetCPUDescriptorHandleForHeapStart() };
        handle.ptr += ctx.m_samplerHeapCurrentOffset * ctx.m_samplerHeapIncrement;

        // Creates sampler
        device->CreateSampler(&desc, handle);

        // Increments offset for next descriptor
        ctx.m_samplerHeapCurrentOffset++;
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

#endif