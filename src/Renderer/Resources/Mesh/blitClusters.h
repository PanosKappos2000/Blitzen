#pragma once
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
    // CE_MAX_CLUSTER_PER_SURFACE ?????
    constexpr uint32_t CE_MAX_WORLD_CLUSTER_COUNT = 1'000'000;
    constexpr uint32_t CE_MAX_INSTANCE_PER_CLUSTER_COUNT = 100'000;

    constexpr uint32_t CE_MAX_VERTICES_PER_CLUSTER = 64;
    constexpr uint32_t CE_MAX_TRIANGLES_PER_CLUSTER = 124;
    constexpr float CE_CLUSTER_CONE_WEIGHT = 0.25f;

	struct ClusterContainer
	{
        Cluster* m_clusters{ nullptr };
        uint32_t m_clusterCount{ 0 };

        uint32_t* m_clusterIndices{ nullptr };
        uint32_t m_clusterIndicesCount{ 0 };

        HCluster* HLSL_CLUSTERS{ nullptr };

        void ALLOC_HLSL();
        ~ClusterContainer();
	};
}