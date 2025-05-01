#include "dx12Renderer.h"
#include "dx12Pipelines.h"
#include "dx12Resources.h"

namespace BlitzenDX12
{
	uint8_t Dx12Renderer::UploadTexture(void* pData, const char* filepath)
	{
		return 1;
	}

	static uint8_t CreateComputePipelines()
	{
		return 1;
	}

	static uint8_t CreateTestIndirectCmds(ID3D12Device* device, Dx12Renderer::FrameTools& frameTools, ID3D12CommandQueue* commandQueue,
		Dx12Renderer::VarBuffers& buffers, BlitzenEngine::RenderingResources* pResources)
	{
		const uint32_t maxCmdCount = 1000;
		auto pRenders{ pResources->renders };
		uint32_t cmdCount = maxCmdCount;
		auto renderCount{ pResources->renderObjectCount };
		if (cmdCount > renderCount)
		{
			cmdCount = renderCount;
		}

		const auto& surfaces{ pResources->GetSurfaceArray() };
		const auto& lods{ pResources->GetLodData() };
		BlitCL::StaticArray<IndirectDrawCmd, maxCmdCount> cmds;

		for (uint32_t i = 0; i < cmdCount; ++i)
		{
			auto& cmd = cmds[i];
			auto& surface = surfaces[pRenders[i].surfaceId];
			auto& lod = lods[surface.lodOffset];

			cmd.drawId = i;
			cmd.command.IndexCountPerInstance = lod.indexCount;
			cmd.command.StartIndexLocation = lod.firstIndex;
			cmd.command.BaseVertexLocation = 0;
			cmd.command.InstanceCount = 1;
			cmd.command.StartInstanceLocation = 0;
		}

		DX12WRAPPER<ID3D12Resource> cmdStagingBuffer{ nullptr };
		if (!CreateBuffer(device, cmdStagingBuffer.ReleaseAndGetAddressOf(), cmdCount * sizeof(IndirectDrawCmd), D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
		{
			BLIT_ERROR("Failed test function CreateTestIndirectCmds");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> countStagingBuffer{ nullptr };
		if (!CreateBuffer(device, countStagingBuffer.ReleaseAndGetAddressOf(), sizeof(uint32_t), D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_UPLOAD))
		{
			BLIT_ERROR("Failed test function CreateTestIndirectCmds");
			return 0;
		}

		void* pMappedCmd{ nullptr };
		auto mappingRes{ cmdStagingBuffer->Map(0, nullptr, &pMappedCmd) };
		if (FAILED(mappingRes))
		{
			return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
		}
		BlitzenCore::BlitMemCopy(pMappedCmd, cmds.Data(), sizeof(IndirectDrawCmd) * cmdCount);

		void* pMappedCount{ nullptr };
		mappingRes = cmdStagingBuffer->Map(0, nullptr, &pMappedCount);
		if (FAILED(mappingRes))
		{
			return LOG_ERROR_MESSAGE_AND_RETURN(mappingRes);
		}
		BlitzenCore::BlitMemCopy(pMappedCount, &cmdCount, sizeof(uint32_t));

		frameTools.transferCommandAllocator->Reset();
		frameTools.transferCommandList->Reset(frameTools.transferCommandAllocator.Get(), nullptr);

		// Transitions SSBOs to copy dest
		D3D12_RESOURCE_BARRIER copyDestBarriers[2]{};
		CreateResourcesTransitionBarrier(copyDestBarriers[0], buffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[1], buffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		frameTools.transferCommandList->ResourceBarrier(2, copyDestBarriers);

		// Transitions stagingBuffers to copy source
		D3D12_RESOURCE_BARRIER copySourceBarriers[2]{};
		CreateResourcesTransitionBarrier(copySourceBarriers[0], cmdStagingBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[1], countStagingBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		frameTools.transferCommandList->ResourceBarrier(2, copySourceBarriers);

		frameTools.transferCommandList->CopyResource(buffers.indirectDrawBuffer.buffer.Get(), cmdStagingBuffer.Get());
		frameTools.transferCommandList->CopyResource(buffers.indirectDrawCount.buffer.Get(), countStagingBuffer.Get());

		// Switches back to common, because it will be expected to be in that state later
		D3D12_RESOURCE_BARRIER switchBack[2]{};
		CreateResourcesTransitionBarrier(switchBack[0], buffers.indirectDrawBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		CreateResourcesTransitionBarrier(switchBack[1], buffers.indirectDrawCount.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		frameTools.transferCommandList->ResourceBarrier(2, switchBack);

		frameTools.transferCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.transferCommandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		const UINT64 fence = frameTools.copyFenceValue++;
		commandQueue->Signal(frameTools.copyFence.Get(), fence);
		// Waits for the previous frame
		if (frameTools.copyFence->GetCompletedValue() < fence)
		{
			frameTools.copyFence->SetEventOnCompletion(fence, frameTools.copyFenceEvent);
			WaitForSingleObject(frameTools.copyFenceEvent, INFINITE);
		}

		return 1;
	}

	static uint8_t CreateRootSignatures(ID3D12Device* device, ID3D12RootSignature** ppOpaqueRootSignature)
	{
		D3D12_DESCRIPTOR_RANGE opaqueSrvRanges[Ce_OpaqueSrvRangeCount]{};
		CreateDescriptorRange(opaqueSrvRanges[Ce_VertexBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_VertexBufferDescriptorCount, Ce_VertexBufferRegister);

		D3D12_DESCRIPTOR_RANGE sharedSrvRanges[Ce_SharedSrvRangeCount]{};
		CreateDescriptorRange(sharedSrvRanges[Ce_SurfaceBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_SurfaceBufferDescriptorCount, Ce_SurfaceBufferRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_TransformBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_TransformBufferDescriptorCount, Ce_TransformBufferRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_RenderObjectBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_RenderObjectBufferDescriptorCount, Ce_RenderObjectBufferRegister);
		CreateDescriptorRange(sharedSrvRanges[Ce_IndirectDrawBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_UAV, Ce_IndirectDrawBufferDescriptorCount, Ce_IndirectDrawBufferRegister);

		D3D12_ROOT_PARAMETER rootParameters[4] = {};
		CreateRootParameterDescriptorTable(rootParameters[0], opaqueSrvRanges, Ce_OpaqueSrvRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterDescriptorTable(rootParameters[1], sharedSrvRanges, Ce_SharedSrvRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterCBV(rootParameters[2], Ce_ViewDataBufferRegister, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterPushConstants(rootParameters[3], 1, 0, 1, D3D12_SHADER_VISIBILITY_VERTEX);

		if (!CreateRootSignature(device, ppOpaqueRootSignature, 4, rootParameters))
		{
			BLIT_ERROR("Failed to create opaque root signature");
			return 0;
		}

		// success
		return 1;
	}

	static uint8_t CreateCmdSignatures(ID3D12Device* device, ID3D12RootSignature* opaqueRootSignature, ID3D12CommandSignature** ppOpaqueCmdSignature)
	{
		D3D12_INDIRECT_ARGUMENT_DESC indirectDescs[2]{};
		indirectDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		indirectDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		indirectDescs[1].Constant.DestOffsetIn32BitValues = 0;
		indirectDescs[1].Constant.Num32BitValuesToSet = 1;
		indirectDescs[1].Constant.RootParameterIndex = 3;

		D3D12_COMMAND_SIGNATURE_DESC sigDesc{};
		sigDesc.NodeMask = 0;
		sigDesc.NumArgumentDescs = 1;
		sigDesc.pArgumentDescs = indirectDescs;
		sigDesc.ByteStride = sizeof(IndirectDrawCmd);
		auto opaqueCmdRes{ device->CreateCommandSignature(&sigDesc, nullptr, IID_PPV_ARGS(ppOpaqueCmdSignature)) };
		if (FAILED(opaqueCmdRes))
		{
			return LOG_ERROR_MESSAGE_AND_RETURN(opaqueCmdRes);
		}

		// success
		return 1;
	}

	static uint8_t CreateGraphicsPipelines(ID3D12Device* device, ID3D12RootSignature* opaqueGrahpicsRootSignature, ID3D12PipelineState** ppPso)
	{
		if (!CreateOpaqueGraphicsPipeline(device, opaqueGrahpicsRootSignature, ppPso))
		{
			BLIT_ERROR("Failed to create opaque grahics pipeline");
			return 0;
		}

		return 1;
	}

	static uint8_t CopyDataToVarSSBOs(ID3D12Device* device, Dx12Renderer::FrameTools& frameTools, Dx12Renderer::VarBuffers& buffers,
		ID3D12CommandQueue* commandQueue)
	{
		frameTools.transferCommandAllocator->Reset();
		frameTools.transferCommandList->Reset(frameTools.transferCommandAllocator.Get(), nullptr);

		// Transitions SSBOs to copy dest
		D3D12_RESOURCE_BARRIER copyDestBarriers[Ce_VarSSBODataCount]{};
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_TransformStagingBufferIndex], buffers.transformBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		frameTools.transferCommandList->ResourceBarrier(Ce_VarSSBODataCount, copyDestBarriers);

		// Transitions stagingBuffers to copy source
		D3D12_RESOURCE_BARRIER copySourceBarriers[Ce_VarSSBODataCount]{};
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_TransformStagingBufferIndex], buffers.transformBuffer.staging.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		frameTools.transferCommandList->ResourceBarrier(BLIT_ARRAY_SIZE(copySourceBarriers), copySourceBarriers);

		frameTools.transferCommandList->CopyResource(buffers.transformBuffer.buffer.Get(), buffers.transformBuffer.staging.Get());

		frameTools.transferCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.transferCommandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		const UINT64 fence = frameTools.copyFenceValue++;
		commandQueue->Signal(frameTools.copyFence.Get(), fence);
		// Waits for the previous frame
		if (frameTools.copyFence->GetCompletedValue() < fence)
		{
			frameTools.copyFence->SetEventOnCompletion(fence, frameTools.copyFenceEvent);
			WaitForSingleObject(frameTools.copyFenceEvent, INFINITE);
		}

		return 1;
	}

	static uint8_t CreateVarBuffers(ID3D12Device* device, ID3D12CommandQueue* commandQueue, Dx12Renderer::FrameTools& frameTools, 
		Dx12Renderer::VarBuffers* varBuffers, BlitzenEngine::RenderingResources* pResources)
	{
		const auto& transforms{ pResources->transforms };
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& buffers = varBuffers[i];

			if (!CreateCBuffer(device, buffers.viewDataBuffer))
			{
				BLIT_ERROR("Failed to create view data buffer");
				return 0;
			}

			if (!CreateVarSSBO(device, buffers.transformBuffer, transforms.GetSize(), transforms.Data()))
			{
				BLIT_ERROR("Failed to create transform buffer");
				return 0;
			}

			UINT64 indirectBufferSize{ 1000 * sizeof(IndirectDrawCmd) };
			if (!CreateBuffer(device, buffers.indirectDrawBuffer.buffer.ReleaseAndGetAddressOf(), indirectBufferSize,
				D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("Failed to create indirect draw buffer");
				return 0;
			}

			if (!CreateBuffer(device, buffers.indirectDrawCount.buffer.ReleaseAndGetAddressOf(), sizeof(uint32_t),
				D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
			{
				BLIT_ERROR("Failed to create indirect count buffer");
				return 0;
			}

			frameTools.transferCommandAllocator->Reset();
			frameTools.transferCommandList->Reset(frameTools.transferCommandAllocator.Get(), nullptr);

			D3D12_RESOURCE_BARRIER copyBarriers[2]{};
			CreateResourcesTransitionBarrier(copyBarriers[0], buffers.transformBuffer.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			CreateResourcesTransitionBarrier(copyBarriers[1], buffers.transformBuffer.staging.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
			frameTools.transferCommandList->ResourceBarrier(BLIT_ARRAY_SIZE(copyBarriers), copyBarriers);

			frameTools.transferCommandList->CopyResource(buffers.transformBuffer.buffer.Get(), buffers.transformBuffer.staging.Get());

			frameTools.transferCommandList->Close();
			ID3D12CommandList* commandLists[] = { frameTools.transferCommandList.Get() };
			commandQueue->ExecuteCommandLists(1, commandLists);

			const UINT64 fence = frameTools.copyFenceValue++;
			commandQueue->Signal(frameTools.copyFence.Get(), fence);
			// Waits for the previous frame
			if (frameTools.copyFence->GetCompletedValue() < fence)
			{
				frameTools.copyFence->SetEventOnCompletion(fence, frameTools.copyFenceEvent);
				WaitForSingleObject(frameTools.copyFenceEvent, INFINITE);
			}
		}
		// Success
		return 1;
	}

	struct SSBOCopyContext
	{
		ID3D12Resource* stagingBuffers[Ce_ConstDataSSBOCount];
		UINT64 stagingBufferSizes[Ce_ConstDataSSBOCount];
	};
	static uint8_t CopyDataToSSBOs(ID3D12Device* device, Dx12Renderer::FrameTools& frameTools, Dx12Renderer::ConstBuffers& buffers, 
		SSBOCopyContext& context, ID3D12CommandQueue* commandQueue)
	{
		frameTools.transferCommandAllocator->Reset();
		frameTools.transferCommandList->Reset(frameTools.transferCommandAllocator.Get(), nullptr);

		// Transitions SSBOs to copy dest
		D3D12_RESOURCE_BARRIER copyDestBarriers[Ce_ConstDataSSBOCount]{};
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_VertexStagingBufferIndex], buffers.vertexBuffer.buffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_IndexStagingBufferIndex], buffers.indexBuffer.Get(), 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_SurfaceStagingBufferIndex], buffers.surfaceBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		CreateResourcesTransitionBarrier(copyDestBarriers[Ce_RenderStagingBufferIndex], buffers.renderBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		frameTools.transferCommandList->ResourceBarrier(Ce_ConstDataSSBOCount, copyDestBarriers);

		// Transitions stagingBuffers to copy source
		D3D12_RESOURCE_BARRIER copySourceBarriers[Ce_ConstDataSSBOCount]{};
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_VertexStagingBufferIndex], context.stagingBuffers[Ce_VertexStagingBufferIndex], 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_IndexStagingBufferIndex], context.stagingBuffers[Ce_IndexStagingBufferIndex],
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_SurfaceStagingBufferIndex], context.stagingBuffers[Ce_SurfaceStagingBufferIndex],
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_RenderStagingBufferIndex], context.stagingBuffers[Ce_RenderStagingBufferIndex],
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		frameTools.transferCommandList->ResourceBarrier(BLIT_ARRAY_SIZE(copySourceBarriers), copySourceBarriers);

		frameTools.transferCommandList->CopyResource(buffers.vertexBuffer.buffer.Get(), context.stagingBuffers[Ce_VertexStagingBufferIndex]);
		frameTools.transferCommandList->CopyResource(buffers.indexBuffer.Get(), context.stagingBuffers[Ce_IndexStagingBufferIndex]);
		frameTools.transferCommandList->CopyResource(buffers.surfaceBuffer.buffer.Get(), context.stagingBuffers[Ce_SurfaceStagingBufferIndex]);
		frameTools.transferCommandList->CopyResource(buffers.renderBuffer.buffer.Get(), context.stagingBuffers[Ce_RenderStagingBufferIndex]);

		frameTools.transferCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.transferCommandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		const UINT64 fence = frameTools.copyFenceValue++;
		commandQueue->Signal(frameTools.copyFence.Get(), fence);
		// Waits for the previous frame
		if (frameTools.copyFence->GetCompletedValue() < fence)
		{
			frameTools.copyFence->SetEventOnCompletion(fence, frameTools.copyFenceEvent);
			WaitForSingleObject(frameTools.copyFenceEvent, INFINITE);
		}

		return 1;
	}

	static uint8_t CreateConstBuffers(ID3D12Device* device, Dx12Renderer::FrameTools& frameTools, ID3D12CommandQueue* commandQueue,
		BlitzenEngine::RenderingResources* pResources, Dx12Renderer::ConstBuffers& buffers)
	{
		const auto& vertices{ pResources->GetHlslVertices() };
		const auto& indices{ pResources->GetIndicesArray() };
		const auto& surfaces{ pResources->GetSurfaceArray() };
		BlitzenEngine::RenderObject* renders{ pResources->renders };
		auto renderObjectCount{ pResources->renderObjectCount };

		DX12WRAPPER<ID3D12Resource> vertexStagingBuffer{ nullptr };
		UINT64 vertexBufferSize{ CreateSSBO(device, buffers.vertexBuffer, vertexStagingBuffer, vertices.GetSize(), vertices.Data())};
		if (!vertexBufferSize)
		{
			BLIT_ERROR("Failed to create vertex buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> indexStagingBuffer{ nullptr };
		UINT64 indexBufferSize{ CreateIndexBuffer(device, buffers.indexBuffer, 
			indexStagingBuffer, indices.GetSize(), indices.Data(), buffers.indexBufferView)};
		if (!indexBufferSize)
		{
			BLIT_ERROR("Failed to create index buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> surfaceStagingBuffer{ nullptr };
		UINT64 surfaceBufferSize{ CreateSSBO(device, buffers.surfaceBuffer, surfaceStagingBuffer, surfaces.GetSize(), surfaces.Data()) };
		if (!surfaceBufferSize)
		{
			BLIT_ERROR("Failed to create surface buffer");
			return 0;
		}

		DX12WRAPPER<ID3D12Resource> renderStagingBuffer{ nullptr };
		UINT64 renderBufferSize{ CreateSSBO(device, buffers.renderBuffer, renderStagingBuffer, renderObjectCount, renders) };
		if(!renderBufferSize)
		{
			BLIT_ERROR("Failed to create render buffer");
			return 0;
		}

		SSBOCopyContext copyContext;
		copyContext.stagingBuffers[Ce_VertexStagingBufferIndex] = vertexStagingBuffer.Get();
		copyContext.stagingBuffers[Ce_IndexStagingBufferIndex] = indexStagingBuffer.Get();
		copyContext.stagingBuffers[Ce_SurfaceStagingBufferIndex] = surfaceStagingBuffer.Get();
		copyContext.stagingBuffers[Ce_RenderStagingBufferIndex] = renderStagingBuffer.Get();
		copyContext.stagingBufferSizes[Ce_VertexStagingBufferIndex] = vertexBufferSize;
		copyContext.stagingBufferSizes[Ce_IndexStagingBufferIndex] = indexBufferSize;
		copyContext.stagingBufferSizes[Ce_SurfaceStagingBufferIndex] = surfaceBufferSize;
		copyContext.stagingBufferSizes[Ce_RenderStagingBufferIndex] = renderBufferSize;
		if (!CopyDataToSSBOs(device, frameTools, buffers, copyContext, commandQueue))
		{
			BLIT_ERROR("Failed to copy data to GPU");
			return 0;
		}

		// Success
		return 1;
	}

	static void CreateResourceViews(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, Dx12Renderer::DescriptorContext& descriptorContext,
		Dx12Renderer::ConstBuffers& staticBuffers, Dx12Renderer::VarBuffers* varBuffers, BlitzenEngine::RenderingResources* pResources)
	{
		const auto& vertices{ pResources->GetHlslVertices() };
		const auto& transforms{ pResources->transforms };
		const auto& surfaces{ pResources->GetSurfaceArray() };
		BlitzenEngine::RenderObject* pRenders{ pResources->renders };
		auto renderCount{ pResources->renderObjectCount };

		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors and begins
			descriptorContext.opaqueSrvOffset[i] = descriptorContext.srvHeapOffset;
			auto& vars = varBuffers[i];

			CreateBufferShaderResourceView(device, staticBuffers.vertexBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, staticBuffers.vertexBuffer.srvDesc[i], (UINT)vertices.GetSize(), sizeof(BlitzenEngine::Vertex));
		}

		// Teams of descriptors used by both graphics and compute pipelines
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors and begins
			descriptorContext.sharedSrvOffset[i] = descriptorContext.srvHeapOffset;
			auto& vars = varBuffers[i];

			CreateBufferShaderResourceView(device, staticBuffers.surfaceBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, staticBuffers.surfaceBuffer.srvDesc[i], (UINT)surfaces.GetSize(), sizeof(BlitzenEngine::PrimitiveSurface));

			CreateBufferShaderResourceView(device, vars.transformBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, vars.transformBuffer.srvDesc, (UINT)transforms.GetSize(), sizeof(BlitzenEngine::MeshTransform));

			CreateBufferShaderResourceView(device, staticBuffers.renderBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, staticBuffers.renderBuffer.srvDesc[i], renderCount, sizeof(BlitzenEngine::RenderObject));

			CreateUnorderedAccessView(device, vars.indirectDrawBuffer.buffer.Get(), vars.indirectDrawCount.buffer.Get(),
				srvHeap->GetCPUDescriptorHandleForHeapStart(), descriptorContext.srvHeapOffset, 1000, sizeof(BlitzenDX12::IndirectDrawCmd), 0);

			//CreateUnorderedAccessView(device, vars.indirectDrawCount.buffer.Get(), nullptr, srvHeap->GetCPUDescriptorHandleForHeapStart(), 
			//	descriptorContext.srvHeapOffset, 1, sizeof(uint32_t), 0); CAREFUL, this is compute only
		}

		// Saves the offset of this group of descriptors and begins
		descriptorContext.sharedCbvOffset = descriptorContext.srvHeapOffset;
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& vars = varBuffers[i];

			vars.viewDataBuffer.cbvDesc = {};
			vars.viewDataBuffer.cbvDesc.BufferLocation = vars.viewDataBuffer.buffer->GetGPUVirtualAddress();
			vars.viewDataBuffer.cbvDesc.SizeInBytes = sizeof(BlitzenEngine::CameraViewData);
			auto descriptorHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
			descriptorHandle.ptr += descriptorContext.srvHeapOffset * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			device->CreateConstantBufferView(&vars.viewDataBuffer.cbvDesc, descriptorHandle);
			descriptorContext.srvHeapOffset++;
		}
	}

	uint8_t Dx12Renderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources, 
	float& pyramidWidth, float& pyramidHeight)
	{
		pResources->GenerateHlslVertices();

		if (!CreateRootSignatures(m_device.Get(), m_opaqueRootSignature.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create root signatures");
			return 0;
		}

		if (!CreateCmdSignatures(m_device.Get(), m_opaqueRootSignature.Get(), m_opaqueCmdSingature.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create command signatures");
			return 0;
		}

		if (!CreateConstBuffers(m_device.Get(), m_frameTools[m_currentFrame], m_transferCommandQueue.Get(), pResources, m_constBuffers))
		{
			BLIT_ERROR("Failed to create constant buffers");
			return 0;
		}

		if (!CreateVarBuffers(m_device.Get(), m_transferCommandQueue.Get(), m_frameTools[m_currentFrame], m_varBuffers, pResources))
		{
			BLIT_ERROR("Failed to create var buffers");
			return 0;
		}

		CreateResourceViews(m_device.Get(), m_srvHeap.Get(), m_descriptorContext, 
			m_constBuffers, m_varBuffers, pResources);
		if (!CheckForDeviceRemoval(m_device.Get()))
		{
			return 0;
		}

		if (!CreateGraphicsPipelines(m_device.Get(), m_opaqueRootSignature.Get(), m_opaqueGraphicsPso.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create graphics pipelines");
			return 0;
		}

		for (auto& buffers : m_varBuffers)
		{
			if (!CreateTestIndirectCmds(m_device.Get(), m_frameTools[m_currentFrame], m_transferCommandQueue.Get(), buffers, pResources))
			{
				BLIT_WARN("Failed to load tests for indirect draw buffer");
				BLIT_WARN("This will not cause failure, but it might break the expected tests");
			}
		}

		return 1;
	}

	void Dx12Renderer::FinalSetup()
	{
		auto& frameTools{ m_frameTools[m_currentFrame] };

		frameTools.mainGraphicsCommandAllocator->Reset();
		frameTools.mainGraphicsCommandList->Reset(frameTools.mainGraphicsCommandAllocator.Get(), nullptr);

		D3D12_RESOURCE_BARRIER staticBufferBarriers[Ce_ConstDataSSBOCount]{};
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_VertexStagingBufferIndex], m_constBuffers.vertexBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_IndexStagingBufferIndex], m_constBuffers.indexBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_SurfaceStagingBufferIndex], m_constBuffers.surfaceBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		CreateResourcesTransitionBarrier(staticBufferBarriers[Ce_RenderStagingBufferIndex], m_constBuffers.renderBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		frameTools.mainGraphicsCommandList->ResourceBarrier(Ce_ConstDataSSBOCount, staticBufferBarriers);

		/*
			Creates a barrier for each copy of a var buffer. Uses varBufferId to keep track of the array element
		*/
		uint32_t varBufferId = 0;
		D3D12_RESOURCE_BARRIER varBuffersFinalState[4 * ce_framesInFlight];
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			CreateResourcesTransitionBarrier(varBuffersFinalState[varBufferId], m_varBuffers[i].transformBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			varBufferId++;
		}

		// TODO: Consider making the viewDataBuffer gpu only, there alot of reads to it in the shaders, I am not feeling it.
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			CreateResourcesTransitionBarrier(varBuffersFinalState[varBufferId], m_varBuffers[i].viewDataBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ); // Might want to change this design later
			varBufferId++;
		}

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			// TODO: temporary before compute shaders, this should be unordered access first
			CreateResourcesTransitionBarrier(varBuffersFinalState[varBufferId], m_varBuffers[i].indirectDrawBuffer.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			varBufferId++;
		}

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			// TODO: temporary before compute shaders, this should be unordered access first
			CreateResourcesTransitionBarrier(varBuffersFinalState[varBufferId], m_varBuffers[i].indirectDrawCount.buffer.Get(),
				D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			varBufferId++;
		}

		frameTools.mainGraphicsCommandList->ResourceBarrier(BLIT_ARRAY_SIZE(varBuffersFinalState), varBuffersFinalState);

		frameTools.mainGraphicsCommandList->Close();
		ID3D12CommandList* commandLists[] = { frameTools.mainGraphicsCommandList.Get() };
		m_commandQueue->ExecuteCommandLists(1, commandLists);

		const UINT64 fence = frameTools.inFlightFenceValue;
		m_commandQueue->Signal(frameTools.inFlightFence.Get(), fence);
		frameTools.inFlightFenceValue++;
		// Waits for the previous frame
		if (frameTools.inFlightFence->GetCompletedValue() < fence)
		{
			frameTools.inFlightFence->SetEventOnCompletion(fence, frameTools.inFlightFenceEvent);
			WaitForSingleObject(frameTools.inFlightFenceEvent, INFINITE);
		}
	}
}