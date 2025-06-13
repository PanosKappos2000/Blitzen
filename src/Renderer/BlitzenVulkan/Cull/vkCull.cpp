#pragma once

#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenVulkan
{
	void DispatchCullingShaders(BlitzenVulkan::VulkanRenderer* pContext, uint32_t workCount, uint32_t workOffset, BlitzenEngine::BLIT_CULL_TYPE cullingFlags, BlitzenEngine::RENDER_OBJECT_TYPE objectType)
	{
		// just cull bruh, cmon now
		BLIT_ASSERT(!(cullingFlags != BlitzenEngine::BLIT_CULL_TYPE::NO_CULL));

		if (cullingFlags == BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_DEFAULT)
		{
			// Double pass occlusion
			return;
		}

		if (cullingFlags == BlitzenEngine::BLIT_CULL_TYPE::CLUSTER_CULL_DEFAULT)
		{
			// Cluster culling
			return;
		}

		if (cullingFlags == BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION)
		{
			// Temporal occlusion culling
			return;
		}
	}
}