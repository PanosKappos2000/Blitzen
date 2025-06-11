#pragma once

#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenVulkan
{
	void DispatchCullingShaders(RendererPtrType pContext, uint32_t workCount, uint32_t workOffset, BlitzenEngine::BLIT_CULL_TYPE cullingFlags, BlitzenEngine::RENDER_OBJECT_TYPE objectType)
	{
		// just cull bruh, cmon now
		BLIT_ASSERT(!(cullingFlags & BLIT_VK_CULL_NOTHING));

		if (cullingFlags & BLIT_VK_CULL_DRAW_DEFAULT)
		{
			// Double pass occlusion
			return;
		}

		if (cullingFlags & BLIT_VK_CULL_CLUSTER_DEFAULT)
		{
			// Cluster culling
			return;
		}

		if (cullingFlags & BLIT_VK_CULL_DRAW_TEMPORAL_OCCLUSION)
		{
			// Temporal occlusion culling
			return;
		}
	}
}