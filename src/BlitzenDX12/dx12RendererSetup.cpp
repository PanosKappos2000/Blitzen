#include "dx12Renderer.h"

namespace BlitzenDX12
{
	void Dx12Renderer::SetupResourceManagement()
	{
		for (auto& tools : m_frameTools)
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			if (device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&tools.commandQueue)) != S_OK)
			{
				m_stats.ce_bResourceManagement = 0;
				return;
			}

			if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&tools.commandAllocator)) != S_OK)
			{
				m_stats.ce_bResourceManagement = 0;
				return;
			}
		}
		m_stats.ce_bResourceManagement = 1;
	}

	uint8_t Dx12Renderer::SetupForRendering(BlitzenEngine::RenderingResources* pResources)
	{
		if (!m_stats.ce_bResourceManagement)
			SetupResourceManagement();
		if (!m_stats.ce_bResourceManagement)
			return 0;

		BLIT_INFO("Dx12 renderer setup")
		return 1;
	}
}