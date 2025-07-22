#pragma once
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	constexpr uint32_t GCTriangleVertices = 3;
	constexpr uint32_t Ce_MaxWorldVertexCount = 30'000'000;
	constexpr uint32_t Ce_MaxWorldVertexIndicesCount = Ce_MaxWorldVertexCount * GCTriangleVertices;
	static_assert(Ce_MaxWorldVertexCount <= Ce_MaxWorldVertexIndicesCount);
	constexpr uint32_t CE_MAX_MESH_PRIMITIVE_VERTEX_COUNT = Ce_MaxWorldVertexCount;
	constexpr uint32_t CE_MAX_MESH_PRIMITIVE_INDEX_COUNT = Ce_MaxWorldVertexIndicesCount;

	constexpr uint32_t GCMaxVertexCountInMeshResource = 3'000'000;
	constexpr uint32_t GCMaxVertexIndicesInMeshResource = 3'000'000;
	static_assert(GCMaxVertexCountInMeshResource <= GCMaxVertexIndicesInMeshResource);

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
	constexpr uint32_t GCGenerateVtxIndicesErrorCode = BlitzenEngine::GCMaxVertexIndicesInMeshResource;
	uint32_t GenerateVtxIndices(uint32_t* destination, uint32_t inedexCount, BlitzenEngine::VtxPos* vtxArr, uint32_t vertexCount);

	struct VTXIDX_OPTIMIZATION_CONTEXT
	{
		uint32_t m_vtxCount{ 0 };
		uint32_t* m_destinationArr{ nullptr };
		uint32_t* m_idxArr{ nullptr };
		uint32_t m_idxCount = 0;
	};

	// --------------------------------------------------------------------------------------------
	// FUNCTION: OptimizeVertexIndices
	//
	// DESCRIPTION:
	//     Reorders the input triangle indices to improve GPU vertex cache locality.
	//     This optimization significantly reduces redundant vertex shader invocations,
	//     especially on large meshes. The algorithm is based on the vertex cache optimization
	//     method from meshoptimizer by Arseny Kapoulkine.
	//
	//     This version is implemented independently for academic clarity, fine-grained
	//     error reporting, and complete control over adjacency and index handling.
	//     It targets Blitzen's split vertex attributes.
	//
	// ALGORITHM OVERVIEW:
	//     - The function uses a simulated FIFO vertex cache (size CE_CACHE_SIZE_MAX).
	//     - Triangles are emitted based on a scoring system that favors cache hits and
	//       minimizes vertex reuse distance.
	//     - A triangle adjacency structure is used to efficiently track neighboring triangles.
	//     - Triangle and vertex scores are continuously updated to prioritize optimal emission.
	//     - When no adjacent triangles remain, the algorithm falls back to a linear scan,
	//       ensuring that all triangles are processed (even in disconnected meshes).
	//
	// MAIN STEPS:
	//     1. Initialization:
	//         - Score tables, cache arrays, emitted flags, and adjacency references are set up.
	//     2. Processing Loop:
	//         - The current triangle is emitted, and its indices are stored.
	//         - Cache is updated with the triangle's vertices.
	//         - The triangle is removed from the adjacency list to reduce future work.
	//     3. Scoring Update:
	//         - Vertices in the cache are rescored based on live triangle counts.
	//         - All connected triangles are updated with the new vertex scores.
	//         - The triangle with the highest score is selected as the next best candidate.
	//     4. Dead-End Handling:
	//         - If no neighbor triangles remain, the algorithm picks the next unprocessed triangle
	//           from the original index stream.
	//     5. Repeat until all triangles have been processed.
	//
	// NOTES:
	//     - CE_CACHE_SIZE_MAX is set to 16 by default (typical GPU post-transform cache size).
	//     - A copy of this function is useful when using split vertex buffers or when exact control
	//       over the mesh processing pipeline is required.
	//     - This function assumes the triangle list is valid and adjacency has been precomputed.
	//
	// WARNING:
	//     This is a performance-critical function; changes to memory layout or scoring logic
	//     may have substantial performance implications on large meshes.
	//
	// REFERENCE:
	//     meshoptimizer (MIT License) by Arseny Kapoulkine
	//     https://github.com/zeux/meshoptimizer
	//
	//
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

	// This is used in dead-end recovery — when the active region in the mesh has no more connected triangles.
	uint32_t GetNextTriangleDeadEnd(uint32_t& inputCursor, BlitzenCore::FAT_BOOL* emittedFlags, uint32_t faceCount);
}