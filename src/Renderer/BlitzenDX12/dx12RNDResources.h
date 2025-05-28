#pragma once

#if defined(_WIN32)

#include "dx12Resources.h"
#include "dx12Context.h"

namespace BlitzenDX12
{
    uint8_t CreateSwapchain(IDXGIFactory6* factory, ID3D12CommandQueue* queue, uint32_t windowWidth, uint32_t windowHeight, HWND hwnd, DX12WRAPPER<IDXGISwapChain3>* pSwapchain);

    // Creates backbuffers and render target view needed to present on the swapchain
    uint8_t CreateSwapchainResources(IDXGISwapChain3* swapchain, ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* backBuffers, DescriptorContext& ctx);

    uint8_t CreateDepthTargets(ID3D12Device* device, DX12WRAPPER<ID3D12Resource>* depthBuffers, DescriptorContext& ctx, uint32_t swapchainWidth, uint32_t swapchainHeight);

    uint8_t CreateDepthPyramidResource(ID3D12Device* device, HI_Z_MAP& hi_z_map, uint32_t width, uint32_t height);

    void CreateDepthPyramidDescriptors(ID3D12Device* device, ReadWriteResources* rwResourcesArray, DescriptorContext& context, DX12WRAPPER<ID3D12Resource>* pDepthTargets, UINT drawWidth, UINT drawHeight);

    void CopyDepthPyramidToSwapchain(ID3D12GraphicsCommandList4* commandList, ID3D12Resource* swapchainBackBuffer, ID3D12Resource* depthPyramid,
        UINT depthPyramidWidth, UINT depthPyramidHeight, ID3D12DescriptorHeap* descriptorHeap, ID3D12CommandQueue* queue, IDXGISwapChain3* swapchain, 
        uint32_t pyramidMip, uint32_t swapchainWidht, uint32_t swapchainHeight);

    UINT64 CreateIndexBuffer(ID3D12Device* device, INDEX_BUFFER& idxBuffer, DX12WRAPPER<ID3D12Resource>& stagingBuffer, size_t elementCount, void* pData);

    template<typename DATA>
    UINT64 CreateSSBO(ID3D12Device* device, SSBO& ssbo, DX12WRAPPER<ID3D12Resource>& stagingBuffer, size_t elementCount, DATA* data, D3D12_RESOURCE_FLAGS ssboFlags = D3D12_RESOURCE_FLAG_NONE)
    {
        if (elementCount == 0)
        {
            BLIT_ERROR("Passed element count 0 to SSBO creation");
            return 0;
        }

        UINT64 ssboSize{ sizeof(DATA) * elementCount };

        if (!CreateBuffer(device, ssbo.buffer.ReleaseAndGetAddressOf(), ssboSize, D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_DEFAULT, ssboFlags))
        {
            BLIT_ERROR("Failed to create SSBO resource");
            return 0;
        }

        if (!CreateBuffer(device, stagingBuffer.ReleaseAndGetAddressOf(), ssboSize, D3D12_RESOURCE_STATE_COMMON,
            D3D12_HEAP_TYPE_UPLOAD))
        {
            BLIT_ERROR("Failed to create SSBO staging buffer");
            return 0;
        }

        void* pMappedData{ nullptr };
        HRESULT mappingRes{ stagingBuffer->Map(0, nullptr, &pMappedData) };
        if (FAILED(mappingRes))
        {
            BLIT_ERROR("Failed to map pointer to staging buffer");
            return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }

        BlitzenCore::BlitMemCopy(pMappedData, data, ssboSize);

        // Success
        return ssboSize;
    }

    template<typename DATA>
    UINT64 CreateCPUDataSSBO(ID3D12Device* device, CPU_WRITE_SSBO& ssbo, DX12WRAPPER<ID3D12Resource>& tempStaging, size_t elementCount, DATA* data, UINT persistentStagingElementCount)
    {
        if (elementCount == 0)
        {
            BLIT_ERROR("Passed element count 0 to SSBO creation");
            return 0;
        }

        UINT64 ssboSize{ sizeof(DATA) * elementCount };

        if (!CreateBuffer(device, ssbo.buffer.ReleaseAndGetAddressOf(), ssboSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT))
        {
            BLIT_ERROR("Failed to create SSBO resource");
            return 0;
        }

        if (!CreateBuffer(device, tempStaging.ReleaseAndGetAddressOf(), ssboSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
        {
            BLIT_ERROR("Failed to create initial staging buffer");
            return 0;
        }

        void* pMappedData{ nullptr };
        HRESULT mappingRes{ tempStaging->Map(0, nullptr, &pMappedData) };
        if (FAILED(mappingRes))
        {
            BLIT_ERROR("Failed to map pointer to staging buffer");
            return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }

        BlitzenCore::BlitMemCopy(pMappedData, data, ssboSize);

        // PERSISTENT CPU DATA
        if (persistentStagingElementCount == 0)
        {
            BLIT_ERROR("Passed size 0 for persistent staging buffer");
            return 0;
        }

        UINT64 stagingSize{ sizeof(DATA) * persistentStagingElementCount };

        if (!CreateBuffer(device, ssbo.staging.ReleaseAndGetAddressOf(), stagingSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
        {
            BLIT_ERROR("Failed to Create peristent staging buffer for CPU WRITE SSBO");
            return 0;
        }

        mappingRes = ssbo.staging->Map(0, nullptr, &ssbo.pData);
        if (FAILED(mappingRes))
        {
            BLIT_ERROR("Failed to map pointer to persistent staging buffer");
            return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }

        ssbo.dataCopySize = stagingSize;
        BlitzenCore::BlitMemCopy(ssbo.pData, data, ssbo.dataCopySize);

        // Success
        return ssboSize;
    }

    template<typename DATA>
    uint8_t CreateCBuffer(ID3D12Device* device, CBUFFER<DATA>& cBuffer)
    {
        if (!CreateBuffer(device, cBuffer.buffer.ReleaseAndGetAddressOf(), sizeof(DATA), D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
        {
            BLIT_ERROR("Failed to create CBUFFER");
            return 0;
        }

        HRESULT mappingRes{ cBuffer.buffer->Map(0, nullptr, &(reinterpret_cast<void*>(cBuffer.pData))) };
        if (FAILED(mappingRes))
        {
            BLIT_ERROR("Failed to map pointer to CBUFFER");
            return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }

        // Success
        return 1;
    }
}

#endif