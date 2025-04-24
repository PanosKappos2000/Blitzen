#include "dx12Renderer.h"

namespace BlitzenDX12
{
    uint8_t Dx12Renderer::FrameTools::Init()
    {
        return 1;
    }

    uint8_t CreateCommandQueue(ID3D12Device* device, ID3D12CommandQueue** ppQueue, 
        D3D12_COMMAND_QUEUE_FLAGS flags, D3D12_COMMAND_LIST_TYPE type, 
        INT priority /*=D3D12_COMMAND_QUEUE_PRIORITY_NORMAL*/, UINT mask /*=0*/)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = flags;
        queueDesc.Type = type;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;// Hardcoded for now
		queueDesc.NodeMask = mask;

		if (!CheckForDeviceRemoval(device))
		{
			BLIT_ERROR("Device removed");
			return 0;
		}

		auto res = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(ppQueue));
        if (FAILED(res))
        {
            _com_error err{ res };
			BLIT_ERROR("Failed to create command queue, Dx12Error: %s", err.ErrorMessage());
			return 0;
		}

        // success
        return 1;
    }
}