#if defined(_WIN32)
#pragma once
#include "Renderer/BlitzenDX12/Context/dx12Context.h"

namespace BlitzenDX12
{
	void DrawCountReset(ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* resetRoot, ID3D12PipelineState* resetPso, D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, ReadWriteResources& rwResources);
}

#endif