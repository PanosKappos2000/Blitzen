#include "blitTriangle.h"
#include "Core/blitMemory.h"
#include "blitClusters.h"

namespace BlitzenEngine
{
	void PrimitiveContainer::ALLOC()
	{
		m_vertices = reinterpret_cast<Vertex*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(Vertex)));
		m_indices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, BlitzenCore::Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t)));

		m_vertexPositions = reinterpret_cast<VtxPos*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(VtxPos)));
		m_vertexUVs = reinterpret_cast<VtxTexCoords*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(VtxTexCoords)));
		m_vertexNormals = reinterpret_cast<VtxNormals*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(VtxNormals)));
		m_vertexTangents = reinterpret_cast<VtxTangents*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(VtxTangents)));
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

		if (m_vertexPositions)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexPositions, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(VtxPos));
		}

		if (m_vertexUVs)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexUVs, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(VtxTexCoords));
		}

		if (m_vertexNormals)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexNormals, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(VtxNormals));
		}

		if (m_vertexTangents)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexTangents, BlitzenCore::Ce_MaxWorldVertexCount * sizeof(VtxTangents));
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