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

	static uint8_t CreateRootSignatures(ID3D12Device* device, ID3D12RootSignature** ppOpaqueRootSignature)
	{
		D3D12_DESCRIPTOR_RANGE opaqueSrvRanges[Ce_OpaqueSrvRangeCount]{};
		CreateDescriptorRange(opaqueSrvRanges[Ce_TransformBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_TransformBufferDescriptorCount, Ce_TransformBufferRegister);
		CreateDescriptorRange(opaqueSrvRanges[Ce_VertexBufferRangeElement], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_VertexBufferDescriptorCount, Ce_VertexBufferRegister);

		D3D12_ROOT_PARAMETER rootParameters[2] = {};
		CreateRootParameterDescriptorTable(rootParameters[0], opaqueSrvRanges, Ce_OpaqueSrvRangeCount, D3D12_SHADER_VISIBILITY_VERTEX);
		CreateRootParameterCBV(rootParameters[1], Ce_ViewDataBufferRegister, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		if (!CreateRootSignature(device, ppOpaqueRootSignature, 2, rootParameters))
		{
			BLIT_ERROR("Failed to create opaque root signature");
			return 0;
		}

		// success
		return 1;
	}

	static uint8_t CreateDescriptorHeaps(ID3D12Device* device, ID3D12DescriptorHeap** ppSrvHeap)
	{
		if (!CreateDescriptorHeap(device, ppSrvHeap, Ce_SrvDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
		{
			BLIT_ERROR("Failed to create srv descriptor heap");
			return 0;
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
		frameTools.transferCommandList->ResourceBarrier(BLIT_ARRAY_SIZE(copyDestBarriers), copyDestBarriers);

		// Transitions stagingBuffers to copy source
		D3D12_RESOURCE_BARRIER copySourceBarriers[Ce_ConstDataSSBOCount]{};
		CreateResourcesTransitionBarrier(copySourceBarriers[Ce_VertexStagingBufferIndex], context.stagingBuffers[Ce_VertexStagingBufferIndex], 
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
		frameTools.transferCommandList->ResourceBarrier(BLIT_ARRAY_SIZE(copySourceBarriers), copySourceBarriers);

		frameTools.transferCommandList->CopyResource(buffers.vertexBuffer.buffer.Get(), context.stagingBuffers[Ce_VertexStagingBufferIndex]);

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
		const auto& vertices{ pResources->GetVerticesArray() };

		DX12WRAPPER<ID3D12Resource> vertexStagingBuffer{ nullptr };
		UINT64 vertexBufferSize{ CreateSSBO(device, buffers.vertexBuffer, vertexStagingBuffer, vertices.GetSize(), vertices.Data())};
		if (!vertexBufferSize)
		{
			BLIT_ERROR("Failed to create vertex buffer");
			return 0;
		}

		SSBOCopyContext copyContext;
		copyContext.stagingBuffers[Ce_VertexStagingBufferIndex] = vertexStagingBuffer.Get();
		copyContext.stagingBufferSizes[Ce_VertexStagingBufferIndex] = vertexBufferSize;
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
		const auto& vertices{ pResources->GetVerticesArray() };
		const auto& transforms{ pResources->transforms };
		
		for (size_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors
			descriptorContext.opaqueSrvOffset = descriptorContext.srvHeapOffset;
			auto& vars = varBuffers[i];

			CreateBufferShaderResourceView(device, vars.transformBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, vars.transformBuffer.srvDesc, (UINT)transforms.GetSize(), sizeof(BlitzenEngine::MeshTransform));

			CreateBufferShaderResourceView(device, staticBuffers.vertexBuffer.buffer.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(),
				descriptorContext.srvHeapOffset, staticBuffers.vertexBuffer.srvDesc[i], (UINT)vertices.GetSize(), sizeof(BlitzenEngine::Vertex));
		}

		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			// Saves the offset of this group of descriptors
			descriptorContext.sharedCbvOffset = descriptorContext.srvHeapOffset;
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
		if (!CreateRootSignatures(m_device.Get(), m_opaqueRootSignature.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create root signatures");
			return 0;
		}

		if (!CreateDescriptorHeaps(m_device.Get(), m_srvHeap.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create descriptor heaps");
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

		if (!CreateGraphicsPipelines(m_device.Get(), m_opaqueRootSignature.Get(), m_opaqueGraphicsPso.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create graphics pipelines");
			return 0;
		}


		return 1;
	}

	void Dx12Renderer::FinalSetup()
	{
		auto& frameTools{ m_frameTools[m_currentFrame] };

		frameTools.mainGraphicsCommandAllocator->Reset();
		frameTools.mainGraphicsCommandList->Reset(frameTools.mainGraphicsCommandAllocator.Get(), nullptr);

		D3D12_RESOURCE_BARRIER vertexBufferBarrier{};
		CreateResourcesTransitionBarrier(vertexBufferBarrier, m_constBuffers.vertexBuffer.buffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		frameTools.mainGraphicsCommandList->ResourceBarrier(1, &vertexBufferBarrier);

		uint32_t varBufferId = 0;
		D3D12_RESOURCE_BARRIER varBuffersFinalState[2 * ce_framesInFlight];
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