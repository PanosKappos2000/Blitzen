#if defined(_WIN32)
#pragma once
#include "dx12Resources.h"
#include "Renderer/BlitzenDX12/Context/dx12Context.h"

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

    UINT64 CreateIndexBuffer(ID3D12Device* device, INDEX_BUFFER& idxBuffer, size_t elementCount);

    uint8_t AddBlitzenLogoDescriptor(ID3D12Device* device, ReadOnlyResources& readOnlies, DescriptorContext& context);

    template<typename DATA>
    UINT64 CreateSSBO(ID3D12Device* device, SSBO& ssbo, size_t elementCount, D3D12_RESOURCE_FLAGS ssboFlags = D3D12_RESOURCE_FLAG_NONE)
    {
        if (elementCount == 0)
        {
            return 0;
        }

        UINT64 ssboSize{ sizeof(DATA) * elementCount };

        if (!CreateBuffer(device, ssbo.buffer.ReleaseAndGetAddressOf(), ssboSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, ssboFlags))
        {
            return 0;
        }

        // Success
        return ssboSize;
    }

    template<class DATA>
    UINT8 CreateStaging(ID3D12Device* device, STAGING<DATA>& staging, size_t elementCount, DATA* pData, UINT dataOffset = 0)
    {
        if (elementCount == 0)
        {
            return 0;
        }

        UINT64 dataSize{ sizeof(DATA) * elementCount };

        if (!CreateBuffer(device, staging.m_buffer.ReleaseAndGetAddressOf(), dataSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
        {
            return 0;
        }

        void* pMapped{ nullptr };
        HRESULT mappingRes{ staging.m_buffer->Map(0, nullptr, &pMapped) };

        if (FAILED(mappingRes))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
        }

        staging.m_pMapped = reinterpret_cast<DATA*>(pMapped);

        if (!staging.m_pMapped)
        {
            return 0;
        }

        if (pData)
        {
            BlitzenCore::BlitMemCopy(staging.m_pMapped, pData + dataOffset, dataSize);
        }

        staging.m_dataSize = dataSize;

        return 1;
    }

    struct CPU_DATA_SSBO_SIZE_INFO
    {
        UINT m_fullSSBOSize{ 0 };
        UINT m_staticDataSize{ 0 };
        UINT m_staticDataOffset{ 0 };
        UINT m_dynamicDataSize{ 0 };
        UINT m_dynamicDataOffset{ 0 };
    };
    template<typename DATA>
    uint8_t CreateCPU_WRITE_SSBO_Stagings(ID3D12Device* device, STAGING<DATA>& staticStaging, STAGING<DATA>& dynamicStaging, DATA* data, CPU_DATA_SSBO_SIZE_INFO& sizeInfo)
    {
        if (sizeInfo.m_fullSSBOSize == 0)
        {
            return 0;
        }

        if (sizeInfo.m_staticDataSize == 0)
        {
            return 0;
        }

        if (sizeInfo.m_dynamicDataSize == 0)
        {
            sizeInfo.m_dynamicDataSize = 1;
        }

        UINT64 staticStagingSize{ sizeof(DATA) * sizeInfo.m_staticDataSize };

        if (!CreateBuffer(device, staticStaging.m_buffer.ReleaseAndGetAddressOf(), staticStagingSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
        {
            return 0;
        }

        void* pStaticMapped{ nullptr };
        HRESULT staticMappingRes{ staticStaging.m_buffer->Map(0, nullptr, &pStaticMapped) };

        if (FAILED(staticMappingRes))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(staticMappingRes);
        }

        staticStaging.m_pMapped = reinterpret_cast<DATA*>(pStaticMapped);

        if (!staticStaging.m_pMapped)
        {
            return 0;
        }
        
        UINT64 dynamicStagingSize{ sizeof(DATA) * sizeInfo.m_dynamicDataSize };

        if (!CreateBuffer(device, dynamicStaging.m_buffer.ReleaseAndGetAddressOf(), dynamicStagingSize, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
        {
            return 0;
        }

        void* pDynamicMapped{ nullptr };
        HRESULT dynamicMappingRes{ dynamicStaging.m_buffer->Map(0, nullptr, &pDynamicMapped) };

        if (FAILED(dynamicMappingRes))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(dynamicMappingRes);
        }

        dynamicStaging.m_pMapped = reinterpret_cast<DATA*>(pDynamicMapped);

        if (!dynamicStaging.m_pMapped)
        {
            return 0;
        }

        BlitzenCore::BlitMemCopy(dynamicStaging.m_pMapped, data + sizeInfo.m_dynamicDataOffset, dynamicStagingSize);
        BlitzenCore::BlitMemCopy(staticStaging.m_pMapped, data + sizeInfo.m_staticDataOffset, staticStagingSize);

        // saves size
        staticStaging.m_dataSize = staticStagingSize;
        dynamicStaging.m_dataSize = dynamicStagingSize;

        // Success
        return 1;
    }
}

#endif