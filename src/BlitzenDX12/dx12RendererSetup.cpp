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
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = 0;
		rootSignatureDesc.pParameters = nullptr;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		auto rootSignatureResult = device->CreateRootSignature(0, &rootSignatureDesc, sizeof(rootSignatureDesc), 
			IID_PPV_ARGS(ppOpaqueRootSignature));
		if (FAILED(rootSignatureResult)) 
		{
			LOG_ERROR_MESSAGE_AND_RETURN(rootSignatureResult);
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
		if (!CreateRootSignatures(m_device.Get(), m_opaqueGraphicsRootSignature.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create opaque graphics root signature");
			return 0;
		}
		if (!CreateGraphicsPipelines(m_device.Get(), m_opaqueGraphicsRootSignature.Get(), m_opaqueGraphicsPipeline.ReleaseAndGetAddressOf()))
		{
			BLIT_ERROR("Failed to create graphics pipelines");
			return 0;
		}
		return 1;
	}
}