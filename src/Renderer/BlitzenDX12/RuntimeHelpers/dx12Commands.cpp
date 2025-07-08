#if defined(_WIN32)
#include "Renderer/BlitzenDX12/Context/dx12Context.h"
#include "Core/DBLog/blitLogger.h"

namespace BlitzenDX12
{
    uint8_t CmdContext::Init(ID3D12Device* device)
    {
        if (!CheckForDeviceRemoval(device))
        {
            return 0;
        }

		HRESULT commandAllocatorRes = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_graphicsCmdAlloc.ReleaseAndGetAddressOf()));
		if (FAILED(commandAllocatorRes))
		{
            BLIT_ERROR("%s: Failed to create graphics command allocator", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return LOG_ERROR_MESSAGE_AND_RETURN(commandAllocatorRes);
		}
        
		HRESULT commandListRes = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_graphicsCmdAlloc.Get(), nullptr, IID_PPV_ARGS(m_graphicsCmdList.ReleaseAndGetAddressOf()));
        if (FAILED(commandListRes))
        {
            BLIT_ERROR("%s: Failed to create graphics command list", BlitzenCore::CE_DX12_SYSTEM_NAME);
			return LOG_ERROR_MESSAGE_AND_RETURN(commandListRes);
        }
        m_graphicsCmdList->Close();

        HRESULT transferCommandAllocatorRes = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(m_copyCmdAlloc.ReleaseAndGetAddressOf()));
        if (FAILED(transferCommandAllocatorRes))
        {
            BLIT_ERROR("%s: Failed to create transfer command allocator", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(transferCommandAllocatorRes);
        }
        
        HRESULT transferCmdListRes = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_copyCmdAlloc.Get(), nullptr, IID_PPV_ARGS(m_copyCmdList.ReleaseAndGetAddressOf()));
        if (FAILED(transferCmdListRes))
        {
            BLIT_ERROR("%s: Failed to create transfer command list", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(transferCmdListRes);
        }
        m_copyCmdList->Close();

		HRESULT computeCommandAllocatorRes = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(m_computeCmdAlloc.ReleaseAndGetAddressOf()));
        if(FAILED(computeCommandAllocatorRes))
        {
            BLIT_ERROR("%s: Failed to create compute command allocator", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(computeCommandAllocatorRes);
		}

		HRESULT computeCmdListRes = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCmdAlloc.Get(), nullptr, IID_PPV_ARGS(m_computeCmdList.ReleaseAndGetAddressOf()));
        if(FAILED(computeCmdListRes))
        {
            BLIT_ERROR("%s: Failed to create compute command list", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(computeCmdListRes);
		}
        m_computeCmdList->Close();

        HRESULT fenceRes = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_frameFence.m_dx12Handle.ReleaseAndGetAddressOf()));
        if (FAILED(fenceRes))
        {
            BLIT_ERROR("%s: Failed to create fence", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(fenceRes);
        }
		m_frameFence.m_value = 1;
		m_frameFence.m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        HRESULT copyFenceRes = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_copyFence.m_dx12Handle.ReleaseAndGetAddressOf()));
        if (FAILED(copyFenceRes))
        {
            BLIT_ERROR("%s: Failed to create copy fence", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(copyFenceRes);
        }
        m_copyFence.m_value = 100;
        m_copyFence.m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		HRESULT computeFenceRes = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_computeFence.m_dx12Handle.ReleaseAndGetAddressOf()));
        if (FAILED(computeFenceRes))
        {
            BLIT_ERROR("%s: Failed to create compute fence", BlitzenCore::CE_DX12_SYSTEM_NAME);
            return LOG_ERROR_MESSAGE_AND_RETURN(computeFenceRes);
        }
		m_computeFence.m_value = 1000;
		m_copyFence.m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

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