#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "blitTriangle.h"
#include "blitClusters.h"

namespace BlitzenEngine
{
	
	constexpr uint32_t CE_MAX_INSTANCES_PER_LOD = 100'000;
	constexpr uint32_t CE_MAX_LOD_COUNT = BlitzenCore::Ce_MaxMeshPrimitivesCount * BlitzenCore::Ce_MaxLodCountPerSurface;

	struct MeshPrimitiveData
	{
		BlitzenCore::BIG_BOOL m_primitiveTransparencyFlags{ BlitzenCore::BB_FALSE };
		uint32_t m_primitiveVertexCount{ 0 };
		uint32_t m_primitiveVertexOffset{ UINT32_MAX };
	};

	enum class SurfaceCreateRes : int8_t
	{
		SUCCESS = 0,
		UNKNOWN = -1,

		SURFACE_VERTICES_COULD_NOT_BE_ADDED = -3,
		SURFACE_INDICES_COULD_NOT_BE_ADDED = -4,
		MAX_SURFACE_COUNT_REACHED = -5,
		LOD_GENERATION_FAILED = -6,

		FATAL = -100
	};

	inline const char* MESH_PRIMITIVE_CREATE_RES_TO_STRING(SurfaceCreateRes res)
	{
		switch (res)
		{
		case SurfaceCreateRes::SUCCESS: return "SurfaceCreateRes::SUCCESS";
		case SurfaceCreateRes::FATAL: return "SurfaceCreateRes::UNKNOWN_FATAL_ERROR";
		case SurfaceCreateRes::MAX_SURFACE_COUNT_REACHED: return "SurfaceCreateRes::MAX_SURFACE_COUNT_REACHED";
		case SurfaceCreateRes::LOD_GENERATION_FAILED: return "SurfaceCreateRes::LOD_GENERATION_FAILED";
		default: return "SurfaceCreateRes::UNKNOWN";
		}
	}

	using MESH_PRIMITIVE_SPECIAL_FLAGS = uint16_t;
	constexpr MESH_PRIMITIVE_SPECIAL_FLAGS MESH_PRIMITIVE_SPECIAL_NONE = 0;
	constexpr MESH_PRIMITIVE_SPECIAL_FLAGS MESH_PRIMITIVE_SPECIAL_TRANSPARENT = 0x5;

	struct MESH_PRIMITIVE_CREATE_CONTEXT
	{
		Vertex* m_vertices{ nullptr };
		uint32_t m_vertexCount{ 0 };
		uint32_t* m_indices{ nullptr };
		uint32_t m_indexCount{ 0 };
		MESH_PRIMITIVE_SPECIAL_FLAGS m_specialFlags{ MESH_PRIMITIVE_SPECIAL_NONE };
		uint32_t m_materialID{ 0 };
	};

	struct MESH_PRIMITIVE_LOD_CREATE_CONTEXT
	{
		MESH_PRIMITIVE_CREATE_CONTEXT* m_pMeshPrimitiveInfo{ nullptr };
		PrimitiveContainer* m_pPrimitives{ nullptr };
		ClusterContainer* m_pClusters{ nullptr };
		uint32_t m_vertexOffset{ 0 };
	};

	struct LOD_CLUSTERS_CREATE_CONTEXT
	{
		Vertex* m_vertices{ nullptr };
		uint32_t m_vertexCount{ 0 };
		uint32_t* m_indices{ nullptr };
		uint32_t m_indicesCount{ 0 };
	};

	struct MeshPrimitivesContainer
	{
		MeshPrimitiveData m_meshPrimitiveData[BlitzenCore::Ce_MaxMeshPrimitivesCount];
		PrimitiveSurface m_meshPrimitives[BlitzenCore::Ce_MaxMeshPrimitivesCount];
		uint32_t m_meshPrimitivesCount{ 0 };

		LodData m_LODs[CE_MAX_LOD_COUNT]{};
		LodInstanceCounter m_lodInstances[CE_MAX_LOD_COUNT]{};
		uint32_t m_LODCount{ 0 };

		SurfaceCreateRes GenerateSurface(PrimitiveContainer& primitives, ClusterContainer& clusters, MESH_PRIMITIVE_CREATE_CONTEXT& context);

		bool GenerateLODs(PrimitiveSurface& surface, MESH_PRIMITIVE_LOD_CREATE_CONTEXT& context);

		// Generates clusters for a given array of vertices and indices
		uint32_t GenerateClusters(LOD_CLUSTERS_CREATE_CONTEXT&  context, uint32_t vertexOffset, ClusterContainer* pClusters);

		// Generates bounding sphere for primitive based on given vertices and indices
		void GenerateBoundingSphere(PrimitiveSurface& surface, MESH_PRIMITIVE_CREATE_CONTEXT& context);

		void GenerateTangents(MESH_PRIMITIVE_CREATE_CONTEXT& context);
	};
}