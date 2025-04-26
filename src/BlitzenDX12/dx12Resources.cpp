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
		if (!CreateDescriptorHeap(device, rtvHeap.ReleaseAndGetAddressOf(), 
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

    uint8_t CreateDescriptorHeap(ID3D12Device* device, ID3D12DescriptorHeap** ppRtvHeap, D3D12_DESCRIPTOR_HEAP_TYPE type, 
        D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = ce_framesInFlight;
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
}