#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Resources/blitShaderShared.h"
#include "Renderer/Resources/Mesh/blitTriangle.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_TERRAIN_VERTICES = 1'000'000;
	constexpr uint32_t CE_MAX_TERRAIN_INDICES = 3 * CE_MAX_TERRAIN_VERTICES;

	class TerrainContainer
	{
	public:
		BlitzenEngine::VtxPos* terrainVertices = nullptr;
		uint32_t terrainVertexCount = 0;
		float* m_heightBufferData = nullptr;
		uint32_t m_heightDataCount = 0;
		uint32_t* terrainIndices = nullptr;
		uint32_t terrainIndexCount = 0;

		// Copies a degraded array of vertices to the full terrain vertices array.
		// Sizeof(VtxPos) does not need to be added to vertex count. It is done inside the function.
		bool AppendVertices(BlitzenEngine::VtxPos* vertices, uint32_t vertexCount);
		// Copies a degraded array of indices to the full terrain indices array.
		// Sizeof(uint32_t) does not need to be added to index count. It is done inside the function.
		bool AppendIndices(uint32_t* indices, uint32_t indexCount);
		// Copies a degraded array of height data to the full height data array.
		// Sizeof(float) does not need to be added to data count. It is done inside the funciton.
		bool AppendHeightData(float* heightArr, uint32_t dataCount);
		void ALLOC();
		void CLEAR();
		~TerrainContainer();
	};
}

namespace BlitGenerator
{
	bool GenerateTerrainVertices(BlitzenEngine::TerrainContainer& container);
}