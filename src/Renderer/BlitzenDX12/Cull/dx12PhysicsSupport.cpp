#if defined(_WIN32)

#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12Resources.h"

namespace BlitzenEngine
{
	void BMPRDispatchBroadPhaseCollision(BlitzenDX12::Dx12Renderer* pRenderer, CollisionWorkConstant* pPushConstantSource)
	{
		UINT currentFrame = pRenderer->m_currentFrame;
		auto cmdList = pRenderer->m_cmdContext[pRenderer->m_currentFrame].m_computeCmdList.Get();
		auto& rwResources = pRenderer->m_rwResources[currentFrame];

		cmdList->SetComputeRootDescriptorTable(pRenderer->m_descriptorContext.mCollisionSupportParameterID, pRenderer->m_descriptorContext.mCollisionSupportTableHandle);
		cmdList->SetPipelineState(pRenderer->m_pipelineContext.MColliderTransformPso.Get());
		cmdList->SetComputeRoot32BitConstants(pRenderer->m_descriptorContext.mCollisionSupportRootConstantID, BlitzenDX12::GCBMPRCollisionWorkCountContant32BitCount,
			pPushConstantSource, 0);

		D3D12_RESOURCE_BARRIER transformBarriers[1]{};
		BlitzenDX12::CreateResourceUAVBarrier(transformBarriers[0], rwResources.m_transformBuffer.buffer.Get());
		cmdList->ResourceBarrier(BLIT_ARRAY_SIZE(transformBarriers), transformBarriers);

		cmdList->Dispatch(BlitML::GetComputeShaderGroupSize(pPushConstantSource->workCount, 64), 1, 1);
	}
}

#endif