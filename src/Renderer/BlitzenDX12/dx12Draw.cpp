#if defined(_WIN32)
#include "Renderer/BlitzenDX12/Context/dx12Renderer.h"
#include "Renderer/BlitzenDX12/Resources/dx12RNDResources.h"
#include "Renderer/BlitzenDX12/Resources/dx12Pipelines.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenDX12
{
	static void DrawCountReset(ID3D12GraphicsCommandList* commandList, ID3D12RootSignature* resetRoot, ID3D12PipelineState* resetPso, D3D12_GPU_DESCRIPTOR_HANDLE cullSrvHandle, 
		ReadWriteResources& rwResources)
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

	static void DrawCullPass(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources, 
		uint32_t objCount, uint32_t frame)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get() };
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

		// Culling barrier, waits for draw count reset and draw command read
		D3D12_RESOURCE_BARRIER cullingBarriers[2]{};
		// Count reset barrier 
		CreateResourceUAVBarrier(cullingBarriers[0], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		// Command read barrier
		CreateResourcesTransitionBarrier(cullingBarriers[1], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawCullRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);

		// Pipeline + constants
		commandList->SetPipelineState(pipelineContext.m_drawCullPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
		// Command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// Counter write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void DrawOccFirstPass(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources, 
		uint32_t objCount, uint32_t frame)
	{
		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get() };
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

		// Culling barrier, waits for draw count reset and draw command read, and draw visibility write as well
		D3D12_RESOURCE_BARRIER cullingBarriers[3]{};
		// Count reset barrier 
		CreateResourceUAVBarrier(cullingBarriers[0], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		// Command read barrier
		CreateResourcesTransitionBarrier(cullingBarriers[1], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// Draw visibility barrier
		CreateResourceUAVBarrier(cullingBarriers[2], rwResources.m_drawVisBuffer.buffer.Get());
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawOccFirstRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccFirstSharedSRVsRootId, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccFirstExclusiveSRVsRootId, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawOccFirstDrawVisUAVRootId, descriptorContext.m_drawVisUANHandle[frame]);

		// Pipeline + constants
		commandList->SetPipelineState(pipelineContext.m_drawOccFirstPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[2]{};
		// command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// count write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void DrawInstanceCullPass(ID3D12GraphicsCommandList* commandList, DescriptorContext& descriptorContext, PipelineContext& pipelineContext, ReadWriteResources& rwResources,
		BlitzenEngine::DrawContext& context, uint32_t frame)
	{
		BLIT_ASSERT_MESSAGE(true, "Instancing is Under Reconstruction");
		uint32_t lodDataCount{ context.m_meshes.m_meshPrimitives.m_LODCount };
		uint32_t objCount{ context.m_pResidents->m_renders.m_opaqueStaticCount };

		// Binds heap for compute
		ID3D12DescriptorHeap* srvHeaps[] = { descriptorContext.m_viewHeap.Get()};
		commandList->SetDescriptorHeaps(1, srvHeaps);

		// Resets Count
		DrawCountReset(commandList, pipelineContext.m_drawCountResetRoot.Get(), pipelineContext.m_drawCountResetPso.Get(), descriptorContext.m_drawCullViewsHandle[frame], rwResources);

		// Blocks instance counter reset
		D3D12_RESOURCE_BARRIER instCounterResetBarrier{};
		//CreateResourceUAVBarrier(instCounterResetBarrier, rwResources.m_instCmdBuffer.buffer.Get());
		commandList->ResourceBarrier(1, &instCounterResetBarrier);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawCullInstRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstSharedSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstAdditionalSRVsRootID, descriptorContext.m_drawCullInstUAVsHandle[frame]);

		// Pipeline + Constants
		commandList->SetPipelineState(pipelineContext.m_drawInstCountResetPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, (UINT)lodDataCount, 0);

		// Resets instance counters
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(uint32_t(lodDataCount), 64), 1, 1);

		// Culling barriers. Waits for draw and instance count reset, instance buffer and draw buffer read
		D3D12_RESOURCE_BARRIER cullingBarriers[4]{};
		// Draw count barrier
		CreateResourceUAVBarrier(cullingBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get());
		// Instance Counter barrier
		//CreateResourceUAVBarrier(cullingBarriers[2], rwResources.m_instCmdBuffer.buffer.Get());
		// Instance barrier
		//CreateResourceUAVBarrier(cullingBarriers[3], rwResources.m_drawInstBuffer.buffer.Get());
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(cullingBarriers), cullingBarriers);

		// Descriptors
		commandList->SetComputeRootSignature(pipelineContext.m_drawCullInstRoot.Get());
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstExclusiveSRVsRootID, descriptorContext.m_drawCullViewsHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetComputeRootDescriptorTable(Ce_DrawCullInstAdditionalSRVsRootID, descriptorContext.m_drawCullInstUAVsHandle[frame]);

		// Pipeline + constants
		commandList->SetPipelineState(pipelineContext.m_drawCullInstPso.Get());
		commandList->SetComputeRoot32BitConstant(Ce_DrawCullDrawCountConstantRootID, objCount, 0);

		// CULL
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize(objCount, 64), 1, 1);

		// Sets commands with instancing
		commandList->Dispatch(BlitML::GetComputeShaderGroupSize((uint32_t)lodDataCount, 64), 1, 1);

		// Block graphics, should wait for command and count write
		D3D12_RESOURCE_BARRIER graphicsBarriers[3]{};
		// command write
		CreateResourcesTransitionBarrier(graphicsBarriers[0], rwResources.m_drawCmdBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// count write
		CreateResourcesTransitionBarrier(graphicsBarriers[1], rwResources.m_drawCmdCounterBuffer.buffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
		// inst write
		//CreateResourcesTransitionBarrier(instanceBuffer)
		// execute
		commandList->ResourceBarrier(BLIT_ARRAY_SIZE(graphicsBarriers), graphicsBarriers);
	}

	static void DrawOpaqueInst(ID3D12GraphicsCommandList* commandList, PipelineContext& pipelineContext, DescriptorContext& descriptorContext,
		ReadOnlyResources& roResources, ReadWriteResources& rwResources, UINT frame)
	{
		ID3D12DescriptorHeap* graphicsHeaps[] = { descriptorContext.m_viewHeap.Get(), descriptorContext.m_samplerHeap.Get() };
		commandList->SetDescriptorHeaps(2, graphicsHeaps);
		commandList->SetGraphicsRootSignature(pipelineContext.m_opaqueDrawInstRoot.Get());

		// DESCRIPTORS
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstExclusiveSRVsRootID, descriptorContext.m_opaqueDrawViewsExclusiveHandle[frame]);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstSharedSRVsRootID, descriptorContext.m_sharedViewHandle[frame]);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstTexSMPRootID, descriptorContext.m_texSmpHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstMatSRVRootID, descriptorContext.m_opaqueDrawPSExclusiveViewsHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstTexSRVRootID, descriptorContext.m_texDescriptorsSRVHandle);
		commandList->SetGraphicsRootDescriptorTable(Ce_OpaqueDrawInstInstSRVRootID, descriptorContext.m_drawCullInstUAVsHandle[frame]);

		commandList->SetPipelineState(pipelineContext.m_opaqueDrawInstPso.Get());

		commandList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetIndexBuffer(&roResources.m_idxBuffer.m_view);

		// DRAW
		commandList->ExecuteIndirect(pipelineContext.m_opaqueDrawInstCmdSign.Get(), Ce_IndirectDrawCmdBufferSize, rwResources.m_drawCmdBuffer.buffer.Get(), 0, 
			rwResources.m_drawCmdCounterBuffer.buffer.Get(), 0);
	}

	void Dx12Renderer::DrawWhileWaiting(float deltaTime)
	{
		auto& cmdContext = m_cmdContext[m_currentFrame];

		PlaceFence(cmdContext.m_frameFence.m_value, m_commandQueue.Get(), cmdContext.m_frameFence.m_dx12Handle.Get(), cmdContext.m_frameFence.m_event);

		m_swapchainIDX = m_swapchain->GetCurrentBackBufferIndex();

		cmdContext.m_graphicsCmdAlloc->Reset();
		cmdContext.m_graphicsCmdList->Reset(cmdContext.m_graphicsCmdAlloc.Get(), m_pipelineContext.m_trianglePso.Get());

		cmdContext.m_graphicsCmdList->SetGraphicsRootSignature(m_pipelineContext.m_triangleRoot.Get());
		DefineViewportAndScissor(cmdContext.m_graphicsCmdList.Get(), (float)m_swapchainWidth, (float)m_swapchainHeight);

		D3D12_RESOURCE_BARRIER attachmentBarrier{};
		CreateResourcesTransitionBarrier(attachmentBarrier, m_swapchainBackBuffers[m_swapchainIDX].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &attachmentBarrier);

		cmdContext.m_graphicsCmdList->OMSetRenderTargets(1, &m_descriptorContext.m_swapchainRtvHandle[m_currentFrame], FALSE, nullptr);

		cmdContext.m_graphicsCmdList->ClearRenderTargetView(m_descriptorContext.m_swapchainRtvHandle[m_currentFrame], BlitzenCore::Ce_DefaultWindowBackgroundColor, 0, nullptr);

		//BlitML::vec3 triangleColor{ 0, 0.8f, 0.4f };
		//frameTools.mainGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 3, &triangleColor, 0);
		cmdContext.m_graphicsCmdList->SetPipelineState(m_pipelineContext.m_trianglePso.Get());
		cmdContext.m_graphicsCmdList->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdContext.m_graphicsCmdList->DrawInstanced(3, 1, 0, 0);

		D3D12_RESOURCE_BARRIER presentBarrier{};
		CreateResourcesTransitionBarrier(presentBarrier, m_swapchainBackBuffers[m_swapchainIDX].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cmdContext.m_graphicsCmdList->ResourceBarrier(1, &presentBarrier);

		cmdContext.m_graphicsCmdList->Close();
		ID3D12CommandList* commandLists[] = { cmdContext.m_graphicsCmdList.Get() };
		m_commandQueue->ExecuteCommandLists(1, commandLists);
	}
}

#endif