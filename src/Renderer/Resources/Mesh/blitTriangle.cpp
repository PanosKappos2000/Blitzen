#include "blitTriangle.h"
#include "Core/blitMemory.h"
#include "blitClusters.h"
#include "Core/DbLog/blitLogger.h"
#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenEngine
{
	void PrimitiveContainer::ALLOC()
	{
		m_vertices = reinterpret_cast<Vertex*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, Ce_MaxWorldVertexCount * sizeof(Vertex)));
		m_indices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t)));

		m_vertexPositions = reinterpret_cast<VtxPos*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, Ce_MaxWorldVertexCount * sizeof(VtxPos)));
		m_vertexUVs = reinterpret_cast<VtxTexCoords*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, Ce_MaxWorldVertexCount * sizeof(VtxTexCoords)));
		m_vertexNormals = reinterpret_cast<VtxNormals*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, Ce_MaxWorldVertexCount * sizeof(VtxNormals)));
		m_vertexTangents = reinterpret_cast<VtxTangents*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, Ce_MaxWorldVertexCount * sizeof(VtxTangents)));
	}

	void PrimitiveContainer::CLEAN()
	{
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertices, Ce_MaxWorldVertexCount * sizeof(Vertex));
		m_vertices = nullptr;
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_indices, Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t));
		m_indices = nullptr;
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexPositions, Ce_MaxWorldVertexCount * sizeof(VtxPos));
		m_vertexPositions = nullptr;
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexUVs, Ce_MaxWorldVertexCount * sizeof(VtxTexCoords));
		m_vertexUVs = nullptr;
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexNormals, Ce_MaxWorldVertexCount * sizeof(VtxNormals));
		m_vertexNormals = nullptr;
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexTangents, Ce_MaxWorldVertexCount * sizeof(VtxTangents));
		m_vertexTangents = nullptr;
	}

	PrimitiveContainer::~PrimitiveContainer()
	{
		if (m_vertices)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertices, Ce_MaxWorldVertexCount * sizeof(Vertex));
		}

		if (m_indices)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_indices, Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t));
		}

		if (m_vertexPositions)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexPositions, Ce_MaxWorldVertexCount * sizeof(VtxPos));
		}

		if (m_vertexUVs)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexUVs, Ce_MaxWorldVertexCount * sizeof(VtxTexCoords));
		}

		if (m_vertexNormals)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexNormals, Ce_MaxWorldVertexCount * sizeof(VtxNormals));
		}

		if (m_vertexTangents)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_vertexTangents, Ce_MaxWorldVertexCount * sizeof(VtxTangents));
		}
	}

	void ClusterContainer::ALLOC()
	{
		m_clusterVertices = reinterpret_cast<ClusterVertices*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(ClusterVertices)));
		m_clusterSpheres = reinterpret_cast<ClusterSphere*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(ClusterSphere)));
		m_clusterCones = reinterpret_cast<ClusterCone*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(ClusterSphere)));

		m_clusters = reinterpret_cast<Cluster*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(Cluster)));
		m_clusterIndices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE, Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t)));
	}

	void ClusterContainer::CLEAN()
	{
		if constexpr (BlitzenCore::Ce_BuildClusters)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusterVertices, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(ClusterVertices));
			m_clusterVertices = nullptr;
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusterSpheres, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(ClusterSphere));
			m_clusterSpheres = nullptr;
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusterCones, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(ClusterCone));
			m_clusterCones = nullptr;
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusters, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(Cluster));
			m_clusters = nullptr;
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusterIndices, Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t));
			m_clusterIndices = nullptr;
		}
	}

	ClusterContainer::~ClusterContainer()
	{
		if (m_clusterVertices)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusterVertices, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(ClusterVertices));
		}

		if (m_clusterSpheres)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusterSpheres, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(ClusterSphere));
		}

		if (m_clusterCones)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusterCones, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(ClusterCone));
		}

		if (m_clusters)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusters, CE_MAX_WORLD_CLUSTER_COUNT * sizeof(Cluster));
		}

		if (m_clusterIndices)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, m_clusterIndices, Ce_MaxWorldVertexIndicesCount * sizeof(uint32_t));
		}
	}

	bool GenerateHlslVertices(PrimitiveContainer& context)
	{
		for (size_t vert = 0; vert < context.m_vertexCount; ++vert)
		{
			const auto& classic = context.m_vertices[vert];

			context.m_vertexPositions[vert] = classic.position;
			context.m_vertexUVs[vert] = VtxTexCoords{ classic.uvX, classic.uvY };
			context.m_vertexNormals[vert] = VtxNormals{ classic.normalX / 127.5f - 1.0f, classic.normalY / 127.5f - 1.0f, classic.normalZ / 127.5f - 1.0f, classic.normalW / 127.5f - 1.0f };
			context.m_vertexTangents[vert] = VtxTangents{ classic.tangentX / 127.5f - 1.0f, classic.tangentY / 127.5f - 1.0f, classic.tangentZ / 127.5f - 1.0f, classic.tangentW / 127.5f - 1.0f };
		}

		return true;
	}

	void ConvertClassicVerticesToHlslFormat(HLSL_VTX_CONTEXT& hlslCtx, Vertex* classicVtxArr, uint32_t count)
	{
		for (uint32_t vert = 0; vert < count; vert++)
		{
			const auto& classic = classicVtxArr[vert];

			hlslCtx.m_vtxPosArr[vert] = classic.position;
			hlslCtx.m_texCoordArr[vert] = VtxTexCoords{ classic.uvX, classic.uvY };
			hlslCtx.m_vtxNrmArr[vert] = VtxNormals{ classic.normalX / 127.5f - 1.0f, classic.normalY / 127.5f - 1.0f, classic.normalZ / 127.5f - 1.0f, classic.normalW / 127.5f - 1.0f };
			hlslCtx.m_vtxTngArr[vert] = VtxTangents{ classic.tangentX / 127.5f - 1.0f, classic.tangentY / 127.5f - 1.0f, classic.tangentZ / 127.5f - 1.0f, classic.tangentW / 127.5f - 1.0f };
		}
	}

	bool GenerateHLSLClusters(ClusterContainer& context)
	{
		for(uint32_t clst = 0; clst < context.m_clusterCount; ++clst)
		{
			const auto& glslClusters{ context.m_clusters[clst] };
		}
		for (uint32_t clst = 0; clst < context.m_clusterCount; ++clst)
		{
			const auto& glslClusters{ context.m_clusters[clst] };

			auto& clusterSphere{ context.m_clusterSpheres[clst]};
			clusterSphere.center = glslClusters.center;
			clusterSphere.radius = glslClusters.radius;

			auto& clusterVertices{ context.m_clusterVertices[clst] };
			clusterVertices.idxCount = glslClusters.triangleCount * 3;
			clusterVertices.idxOffset = glslClusters.dataOffset;

			auto& clusterCone{ context.m_clusterCones[clst] };
			clusterCone.cone = BlitML::vec3{ (float)glslClusters.coneAxisX / 127.f, (float)glslClusters.coneAxisY / 127.f, (float)glslClusters.coneAxisZ / 127.f };
			clusterCone.coneCutoff = glslClusters.coneCutoff / 127.f;
		}

		return true;
	}
}