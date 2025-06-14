#pragma once
#if defined(_WIN32)

#include "Renderer/BlitzenDX12/Context/dx12Context.h"

namespace BlitzenDX12
{
    uint8_t CreateCommandQueue(ID3D12Device* device, ID3D12CommandQueue** ppQueue, D3D12_COMMAND_QUEUE_FLAGS flags, D3D12_COMMAND_LIST_TYPE type,
        INT priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, UINT mask = 0);
}

#endif