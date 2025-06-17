#include "blitTriangle.h"
#include "Core/blitMemory.h"
#include "blitClusters.h"

namespace BlitzenEngine
{
	PrimitiveContainer::PrimitiveContainer()
	{
		m_vertices = reinterpret_cast<Vertex*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(Vertex)));
		m_indices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, BlitzenCore::Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t)));
	}

	PrimitiveContainer::~PrimitiveContainer()
	{
		if (m_vertices)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertices, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(Vertex));
		}

		if (m_indices)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_indices, BlitzenCore::Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t));
		}
	}

	void HLSL_PrimitiveContainer::ALLOC()
	{
		HLSL_VERTICES = reinterpret_cast<HlslVtx*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(HlslVtx)));
	}

	HLSL_PrimitiveContainer::~HLSL_PrimitiveContainer()
	{
		if (HLSL_VERTICES)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, HLSL_VERTICES, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(HlslVtx));
		}
	}

	void ClusterContainer::ALLOC_HLSL()
	{
		HLSL_CLUSTERS = reinterpret_cast<HCluster*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(HCluster)));
	}

	ClusterContainer::~ClusterContainer()
	{
		if (HLSL_CLUSTERS)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, HLSL_CLUSTERS, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(HCluster));
		}

		if (m_clusters)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusters, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(Cluster));
		}

		if (m_clusterIndices)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusterIndices, BlitzenCore::Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t));
		}
	}
}