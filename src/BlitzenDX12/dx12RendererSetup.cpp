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
		D3D12_DESCRIPTOR_RANGE descriptorRanges[Ce_OpaqueGraphicsRangeCount]{};
		CreateDescriptorRange(descriptorRanges[0], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_VertexBufferDescriptorCount, Ce_VertexBufferRegister);
		CreateDescriptorRange(descriptorRanges[1], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_TransformBufferDescriptorCount, Ce_TransformBufferRegister);
		CreateDescriptorRange(descriptorRanges[2], D3D12_DESCRIPTOR_RANGE_TYPE_CBV, Ce_ViewDataBufferDescriptorCount, Ce_ViewDataBufferRegister);

		D3D12_ROOT_PARAMETER rootParameters[1] = {};
		CreateRootParameterDescriptor(rootParameters[0], descriptorRanges, 3/*BLIT_ARRAY_SIZE(descriptorRanges)*/, D3D12_SHADER_VISIBILITY_VERTEX);

		if (!CreateRootSignature(device, ppOpaqueRootSignature, 1, rootParameters))
		{
			BLIT_ERROR("Failed to create opaque root signature");
			return 0;
		}

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

	static uint8_t CreateVarBuffers(ID3D12Device* device, Dx12Renderer::FrameTools& frameTools, Dx12Renderer::VarBuffers* varBuffers)
	{
		for (uint32_t i = 0; i < ce_framesInFlight; ++i)
		{
			auto& buffers = varBuffers[i];

			if (!CreateCBuffer(device, varBuffers->viewDataBuffer))
			{
				BLIT_ERROR("Failed to create view data buffer");
				return 0;
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

	uint8_t Dx12Renderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources, 
	float& pyramidWidth, float& pyramidHeight)
	{
		if (!CreateRootSignatures(m_device.Get(), m_opaqueRootSignature.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create opaque graphics root signature");
			return 0;
		}

		if (!CreateVarBuffers(m_device.Get(), m_frameTools[m_currentFrame], m_varBuffers))
		{
			BLIT_ERROR("Failed to create var buffers");
			return 0;
		}

		if (!CreateConstBuffers(m_device.Get(), m_frameTools[m_currentFrame], m_transferCommandQueue.Get(), pResources, m_constBuffers))
		{
			BLIT_ERROR("Failed to create constant buffers");
			return 0;
		}

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
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		frameTools.mainGraphicsCommandList->ResourceBarrier(1, &vertexBufferBarrier);

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