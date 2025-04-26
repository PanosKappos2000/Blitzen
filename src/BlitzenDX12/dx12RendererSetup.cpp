#include "dx12Renderer.h"
#include "dx12Pipelines.h"

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
		D3D12_DESCRIPTOR_RANGE descriptorRanges[Ce_OpaqueGraphicsRangeCount] = {};
		CreateDescriptorRange(descriptorRanges[0], D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Ce_VertexBufferDescriptorCount, Ce_VertexBufferRegister, 0);

		D3D12_ROOT_PARAMETER rootParameters[1] = {};
		CreateRootParameterDescriptor(rootParameters[0], descriptorRanges, 1/*BLIT_ARRAY_SIZE(descriptorRanges)*/, D3D12_SHADER_VISIBILITY_VERTEX);

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

	uint8_t Dx12Renderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources, 
	float& pyramidWidth, float& pyramidHeight)
	{
		if (!CreateRootSignatures(m_device.Get(), m_opaqueRootSignature.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create opaque graphics root signature");
			return 0;
		}

		if (!CreateGraphicsPipelines(m_device.Get(), m_opaqueRootSignature.Get(), m_opaqueGraphicsPso.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create graphics pipelines");
			return 0;
		}
		return 1;
	}
}