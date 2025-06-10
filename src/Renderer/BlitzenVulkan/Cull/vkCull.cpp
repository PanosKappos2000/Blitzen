#pragma once

#include "vkCull.h"

namespace BlitzenVulkan
{
	void DispatchCullingShaders(VulkanRenderer* pContext, uint32_t workCount, uint32_t workOffset, BLIT_VK_CULLING_FLAGS cullingFlags, BLIT_VK_CULLED_OBJECT_TYPE objectType)
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