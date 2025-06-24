#if defined(_WIN32)
#include "dx12Cull.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"

namespace BlitzenDX12
{
	void DrawCountReset(ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* resetRoot, ID3D12PipelineState* resetPso, D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, ReadWriteResources& rwResources)
	{
		// Reource barrier before count is reset and commands are rewritten
		D3D12_RESOURCE_BARRIER resetBarrier{};
		CreateResourcesTransitionBarrier(resetBarrier, rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(1, &resetBarrier);

		// Descriptors
		commandList->SetComputeRootSignature(resetRoot);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullExclusiveSRVsRootID, cullSrvHandle);

		// Pipeline + constants
		commandList->SetPipelineState(resetPso);

		commandList->Dispatch(1, 1, 1);
	}
}

#endif