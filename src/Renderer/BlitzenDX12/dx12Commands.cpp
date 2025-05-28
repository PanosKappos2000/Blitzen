#if defined(_WIN32)

#include "dx12Renderer.h"

namespace BlitzenDX12
{
    uint8_t Dx12Renderer::FrameTools::Init(ID3D12Device* device)
    {
        if (!CheckForDeviceRemoval(device))
        {
            return 0;
        }

		HRESULT commandAllocatorRes = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mainGraphicsCommandAllocator.ReleaseAndGetAddressOf()));
		if (FAILED(commandAllocatorRes))
		{
            BLIT_ERROR("Failed to create graphics command allocator");
			return LOG_ERROR_MESSAGE_AND_RETURN(commandAllocatorRes);
		}
        
		HRESULT commandListRes = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mainGraphicsCommandAllocator.Get(), nullptr, IID_PPV_ARGS(mainGraphicsCommandList.ReleaseAndGetAddressOf()));
        if (FAILED(commandListRes))
        {
            BLIT_ERROR("Failed to create graphics command list");
			return LOG_ERROR_MESSAGE_AND_RETURN(commandListRes);
        }
        mainGraphicsCommandList->Close();

        HRESULT transferCommandAllocatorRes = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(transferCommandAllocator.ReleaseAndGetAddressOf()));
        if (FAILED(transferCommandAllocatorRes))
        {
            BLIT_ERROR("Failed to create transfer command allocator");
            return LOG_ERROR_MESSAGE_AND_RETURN(transferCommandAllocatorRes);
        }
        
        HRESULT transferCmdListRes = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, transferCommandAllocator.Get(), nullptr, IID_PPV_ARGS(transferCommandList.ReleaseAndGetAddressOf()));
        if (FAILED(transferCmdListRes))
        {
            BLIT_ERROR("Failed to create transfer command list");
            return LOG_ERROR_MESSAGE_AND_RETURN(transferCmdListRes);
        }
        transferCommandList->Close();

        HRESULT dedicatedTransferRes = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(dedicatedTransferAllocator.ReleaseAndGetAddressOf()));
        if (FAILED(dedicatedTransferRes))
        {
            BLIT_ERROR("Failed to create transfer command allocator");
            return LOG_ERROR_MESSAGE_AND_RETURN(dedicatedTransferRes);
        }
        
        dedicatedTransferRes = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, dedicatedTransferAllocator.Get(), nullptr, IID_PPV_ARGS(dedicatedTransferList.ReleaseAndGetAddressOf()));
        if (FAILED(dedicatedTransferRes))
        {
            BLIT_ERROR("Failed to create transfer command list");
            return LOG_ERROR_MESSAGE_AND_RETURN(dedicatedTransferRes);
        }

        HRESULT fenceRes = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(inFlightFence.ReleaseAndGetAddressOf()));
        if (FAILED(fenceRes))
        {
            BLIT_ERROR("Failed to create fence");
            return LOG_ERROR_MESSAGE_AND_RETURN(fenceRes);
        }
		inFlightFenceValue = 1;
		inFlightFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        HRESULT copyFenceRes = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(copyFence.ReleaseAndGetAddressOf()));
        if (FAILED(copyFenceRes))
        {
            BLIT_ERROR("Failed to create copy fence")
            return LOG_ERROR_MESSAGE_AND_RETURN(copyFenceRes);
        }
        copyFenceValue = 100;
        copyFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        // success
        return 1;
    }

    uint8_t CreateCommandQueue(ID3D12Device* device, ID3D12CommandQueue** ppQueue, D3D12_COMMAND_QUEUE_FLAGS flags, D3D12_COMMAND_LIST_TYPE type, 
        INT priority /*=D3D12_COMMAND_QUEUE_PRIORITY_NORMAL*/, UINT mask /*=0*/)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = flags;
        queueDesc.Type = type;
        queueDesc.Priority = priority;
		queueDesc.NodeMask = mask;

		HRESULT res = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(ppQueue));

        if (FAILED(res))
        {
            BLIT_ERROR("Failed to create command queue");
            return LOG_ERROR_MESSAGE_AND_RETURN(res);
		}

        // success
        return 1;
    }
}
#endif