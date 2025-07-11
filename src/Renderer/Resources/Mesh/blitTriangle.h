#pragma once
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_MESH_PRIMITIVE_VERTEX_COUNT = BlitzenCore::Ce_MaxWorldVertexCount;
	constexpr uint32_t CE_MAX_MESH_PRIMITIVE_INDEX_COUNT = BlitzenCore::Ce_MaxWorldVertexIndicesCount;

	class PrimitiveContainer
	{
	public:

		Vertex* m_vertices{ nullptr };
		uint32_t m_vertexCount{ 0 };
		uint32_t* m_indices{ nullptr };
		uint32_t m_vtxIdxCount{ 0 };

		uint32_t m_mapVtxCount{ 0 };
		uint32_t m_mapIdxCount{ 0 };

		VtxPos* m_vertexPositions{ nullptr };
		VtxTexCoords* m_vertexUVs{ nullptr };
		VtxNormals* m_vertexNormals{ nullptr };
		VtxTangents* m_vertexTangents{ nullptr };

		void ALLOC();
		void CLEAN();
		~PrimitiveContainer();

		bool AddVertices(Vertex* vertices, uint32_t count);
		bool AddIndices(uint32_t* indices, uint32_t count);

		bool OverrideVertices(Vertex* vertices, uint32_t count);
		bool OverrideIndices(uint32_t* indices, uint32_t count);
	};

	bool GenerateHlslVertices(PrimitiveContainer& context);

	struct HLSL_VTX_CONTEXT
	{
		VtxPos* m_vtxPosArr{ nullptr };
		VtxNormals* m_vtxNrmArr{ nullptr };
		VtxTangents* m_vtxTngArr{ nullptr };
		VtxTexCoords* m_texCoordArr{ nullptr };
	};
	void ConvertClassicVerticesToHlslFormat(HLSL_VTX_CONTEXT& hlslCtx, Vertex* classicVtxArr, uint32_t count);
}

namespace BlitGenerator
{
	struct VTXIDX_OPTIMIZATION_CONTEXT
	{
		BlitzenEngine::VtxPos* m_vtxPosArr{ nullptr };
		BlitzenEngine::VtxNormals* m_vtxNrmArr{ nullptr };
		BlitzenEngine::VtxTangents* m_vtxTngArr{ nullptr };
		BlitzenEngine::VtxTexCoords* m_vtxUVArr{ nullptr };
		uint32_t m_vtxCount{ 0 };
		uint32_t* m_idxArr{ nullptr };
		uint32_t m_idxCount = 0;
	};

	// Vertex transform cache optimizer
	// Reorders indices to reduce the number of GPU vertex shader invocations
	// If index buffer contains multiple ranges for multiple draw calls, this functions needs to be called on each range individually.
	// Destination must contain enough space for the resulting index buffer (index_count elements)
	bool OptimizeVertexIndices(VTXIDX_OPTIMIZATION_CONTEXT& context);

	struct EDGE_ADJACENCY_CONTEXT
	{
		// Offsets for each vertices's edge in the adjacency list
		uint32_t* m_offsetsArr{ nullptr };

		// The indices of the previous and next vertices for each edge
		uint32_t* m_edgesPreviousIndicesArr{ nullptr };
		uint32_t* m_edgesNextIndicesArr{ nullptr };
	};
	// MESH (current lod) EDGE ADJACENCY.
	// It calculates the connectivity between vertices by organizing edges into an adjacency structure.
	// Specifically, it creates an array of offsets to efficiently access each vertex's connected edges, 
	// as well as two arrays storing the previous and next edges for each edge in the mesh. 
	//
	// - **Edge Collapse**: When simplifying the mesh (e.g., for LOD generation), we need to collapse vertices and remove edges. To do so correctly, we must know which edges are adjacent to each other. 
	// - **Preserving Topology**: Edge adjacency ensures that when vertices are merged, the connected faces and edges are updated, maintaining the mesh's topology.
	// - **Efficient Traversal**: The adjacency structure allows for efficient traversal of the mesh's edges, 
	// which speeds up the simplification process and ensures that we don't repeatedly recompute the relationships between edges.
	bool BuildEdgeAdjacency(EDGE_ADJACENCY_CONTEXT& adjacency, uint32_t* indicesArr, uint32_t indexCount, uint32_t vertexCount);

	struct TRIANGLE_ADJACENCY_CONTEXT
	{
		uint32_t* m_vertexCountersArr;
		uint32_t* m_vertexOffsetsArr;
		uint32_t* m_triangleFaceData;
	};
	// Builds a vertex to triangle lookup table, also called a triangle adjacency list.
	// It allows the optimization algorithm that will come after to quickly answer the question :
	// "Given this vertex, which triangles reference it ?"
	// This will be used in vertex cache optimization to traverse triangles in an order that maximizes vertex reuse, thereby improving GPU vertex cache efficiency.
	bool BuildTriangleAdjacency(TRIANGLE_ADJACENCY_CONTEXT& adjacency, uint32_t* indicesArr, uint32_t indexCount, uint32_t vertexCount);

	constexpr uint32_t CE_CACHE_SIZE_MAX = 16;
	constexpr uint32_t CE_VALENCE_MAX = 8;

	struct VertexScoreTable
	{
		float CACHE[1 + CE_CACHE_SIZE_MAX]; // Cache bonus. Higher score if the vertex is recently used.
		float LIVE[1 + CE_VALENCE_MAX]; // Valence bonus. Higher score if the vertex is near retirement (few triangles left)
	};
	float GetVertexScoreFromTable(int32_t cachePosition, uint32_t liveTriangles);

	uint32_t GetNextTriangleDeadEnd(uint32_t& inputCursor, BlitzenCore::FAT_BOOL* emittedFlags, uint32_t faceCount);
}