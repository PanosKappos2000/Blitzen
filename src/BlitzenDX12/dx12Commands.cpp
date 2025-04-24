#include "dx12Renderer.h"

namespace BlitzenDX12
{
    uint8_t Dx12Renderer::FrameTools::Init(ID3D12Device* device)
    {
		// Command allocator
        if (!CheckForDeviceRemoval(device))
        {
            return 0;
        }
		auto commandAllocatorRes = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, 
            IID_PPV_ARGS(mainGraphicsCommandAllocator.ReleaseAndGetAddressOf()));
		if (FAILED(commandAllocatorRes))
		{
            BLIT_ERROR("Failed to create command allocator");
			return LOG_ERROR_MESSAGE_AND_RETURN(commandAllocatorRes);
		}

        // Command list
        if (!CheckForDeviceRemoval(device))
        {
            return 0;
        }
		auto commandListRes = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			mainGraphicsCommandAllocator.Get(), nullptr, 
            IID_PPV_ARGS(mainGraphicsCommandList.ReleaseAndGetAddressOf()));
        if (FAILED(commandListRes))
        {
            BLIT_ERROR("Failed to create graphics command list");
			return LOG_ERROR_MESSAGE_AND_RETURN(commandListRes);
        }

        // Fence
		if (!CheckForDeviceRemoval(device))
		{
			return 0;
		}
        auto fenceRes = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&inFlightFence));
        if (FAILED(fenceRes))
        {
            BLIT_ERROR("Failed to create fence");
            return LOG_ERROR_MESSAGE_AND_RETURN(fenceRes);
        }
		inFlightFenceValue = 0;
		inFlightFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        // success
        return 1;
    }

    uint8_t CreateCommandQueue(ID3D12Device* device, ID3D12CommandQueue** ppQueue, 
        D3D12_COMMAND_QUEUE_FLAGS flags, D3D12_COMMAND_LIST_TYPE type, 
        INT priority /*=D3D12_COMMAND_QUEUE_PRIORITY_NORMAL*/, UINT mask /*=0*/)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = flags;
        queueDesc.Type = type;
        queueDesc.Priority = priority;
		queueDesc.NodeMask = mask;

		if (!CheckForDeviceRemoval(device))
		{
			return 0;
		}

		auto res = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(ppQueue));
        if (FAILED(res))
        {
            return LOG_ERROR_MESSAGE_AND_RETURN(res);
		}

        // success
        return 1;
    }
}