#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Resources/blitShaderShared.h"
#include "blitTriangle.h"
#include "blitClusters.h"

namespace BlitzenEngine
{
	
	constexpr uint32_t CE_MAX_INSTANCES_PER_LOD = 100'000;
	constexpr uint32_t CE_MAX_LOD_COUNT = BlitzenCore::Ce_MaxMeshPrimitivesCount * BlitzenCore::Ce_MaxLodCountPerSurface;

	enum class SurfaceCreateRes : int8_t
	{
		SUCCESS = 0,
		UNKNOWN = -1,

		SURFACE_VERTICES_COULD_NOT_BE_ADDED = -3,
		SURFACE_INDICES_COULD_NOT_BE_ADDED = -4,
		MAX_SURFACE_COUNT_REACHED = -5,
		LOD_GENERATION_FAILED = -6,
		VERTEX_INDICES_OPTIMIZATION_FAILED = -7,

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
		case SurfaceCreateRes::VERTEX_INDICES_OPTIMIZATION_FAILED: return "SurfaceCreateRes::VERTEX_INDICES_OPTIMIZATION_FAILED";
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
		BlitzenColliderType mColliderType{ BlitzenColliderTypeSphere };
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

	struct MESH_PRIMITIVE_GENERATE_CONTEXT
	{
		HLSL_VTX_CONTEXT* m_pVertexContext{ nullptr };
		uint32_t m_vertexCount{ 0 };
		uint32_t* m_indices{ nullptr };
		uint32_t m_indexCount{ 0 };
		MESH_PRIMITIVE_SPECIAL_FLAGS m_specialFlags{ MESH_PRIMITIVE_SPECIAL_NONE };
		uint32_t m_materialID{ 0 };
		BlitzenColliderType mColliderType{ BlitzenColliderTypeSphere };
	};

	struct MESH_PRIMITIVE_LOD_GENERATE_CONTEXT
	{
		MESH_PRIMITIVE_GENERATE_CONTEXT* m_pMeshPrimitiveInfo{ nullptr };
		PrimitiveContainer* m_pPrimitives{ nullptr };
		ClusterContainer* m_pClusters{ nullptr };
		uint32_t m_vertexOffset{ 0 };
	};

	struct LOD_CLUSTERS_GENERATE_CONTEXT
	{
		HLSL_VTX_CONTEXT* m_pVertexContext{ nullptr };
		uint32_t m_vertexCount{ 0 };
		uint32_t* m_indices{ nullptr };
		uint32_t m_indicesCount{ 0 };
	};

	constexpr uint32_t GCAddSurfaceToMapErrorCode = BlitzenCore::Ce_MaxMeshPrimitivesCount;

	class MeshPrimitivesContainer
	{
	public:
		MeshPrimitiveData m_meshPrimitiveData[BlitzenCore::Ce_MaxMeshPrimitivesCount];
		PrimitiveSurface m_meshPrimitives[BlitzenCore::Ce_MaxMeshPrimitivesCount];
		BoundingSphere m_boundingSpheres[BlitzenCore::Ce_MaxMeshPrimitivesCount];
		SplitColliderDataPair mColliders[BlitzenCore::Ce_MaxMeshPrimitivesCount];
		uint32_t m_meshPrimitivesCount{ 0 };

		LodData m_LODs[CE_MAX_LOD_COUNT]{};
		uint32_t m_LODCount{ 0 };
		uint32_t mMapLodCount{ 0 };

		SurfaceCreateRes GenerateSurface(PrimitiveContainer& primitives, ClusterContainer& clusters, MESH_PRIMITIVE_CREATE_CONTEXT& context);

		SurfaceCreateRes GenerateMeshPrimitive(PrimitiveContainer& primitives, ClusterContainer clusters, MESH_PRIMITIVE_GENERATE_CONTEXT& context);

		bool GenerateLODs(PrimitiveSurface& surface, MESH_PRIMITIVE_LOD_CREATE_CONTEXT& context);

		bool GenerateMeshPrimitiveLODIndices(PrimitiveSurface& surface, MESH_PRIMITIVE_LOD_GENERATE_CONTEXT& context);

		// Generates clusters for a given array of vertices and indices
		uint32_t GenerateClusters(LOD_CLUSTERS_CREATE_CONTEXT&  context, uint32_t vertexOffset, ClusterContainer* pClusters);

		void GenerateTangents(MESH_PRIMITIVE_CREATE_CONTEXT& context);

		uint32_t AddSurfaceToMap();
	};
}

namespace BlitGenerator
{
	// This function calculates the "degradation scale" for Level of Detail (LOD) based on the bounding box size (extent) of the mesh. 
	// The LOD degradation scale is used to determine how much lower the level of detail can go based on the size of the mesh.
	float GetLODDegradationScale(BlitzenEngine::Vertex* vtxArr, uint32_t vertexCount);

	constexpr uint32_t CE_DEGRADATION_ERROR_CODE = UINT32_MAX;
	struct LOD_DEGRADE_CONTEXT
	{
		uint32_t* m_previousIndices{ nullptr };
		uint32_t m_indexCount{ 0 };
		BlitzenEngine::Vertex* m_vertexArr{ nullptr };
		BlitML::vec3* m_vtxNormalsArr{ nullptr };
		BlitML::vec4* m_vtxTangentsArr{ nullptr };
		BlitML::vec2* m_vtxTexCoordsArr{ nullptr };
		uint32_t m_vertexCount{ 0 };
		const float* m_attributeWeightsArr{ nullptr };
		uint32_t m_attribute32BITCount{ 0 };
		const unsigned char* m_vertexLock{ nullptr };
	};
	uint32_t DegradeLevelOfDetail(uint32_t* degradedIndices, uint32_t degradedIndexCount, LOD_DEGRADE_CONTEXT& context, float error, float* pError);
}