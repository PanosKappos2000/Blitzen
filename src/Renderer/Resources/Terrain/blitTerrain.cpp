#include "blitTerrain.h"
#include "Core/blitMemory.h"

namespace BlitzenEngine
{
	bool TerrainContainer::AppendVertices(BlitzenEngine::VtxPos* vertices, uint32_t vertexCount)
	{
		if(vertexCount == 0 || vertices == nullptr || terrainVertexCount + vertexCount > CE_MAX_TERRAIN_VERTICES)
		{
			return false;
		}

		BlitzenCore::MANUAL_COPY(&terrainVertices[terrainVertexCount], vertices, sizeof(VtxPos) * vertexCount);
		terrainVertexCount += vertexCount;
		return true;
	}

	bool TerrainContainer::AppendIndices(uint32_t* indices, uint32_t indexCount)
	{
		if (indexCount == 0 || indices == nullptr || terrainIndexCount + indexCount > CE_MAX_TERRAIN_INDICES)
		{
			return false;
		}

		BlitzenCore::MANUAL_COPY(&terrainIndices[terrainIndexCount], indices, sizeof(uint32_t) * indexCount);
		terrainIndexCount += indexCount;
		return true;
	}

	void TerrainContainer::ALLOC()
	{
		terrainVertices = reinterpret_cast<VtxPos*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Terrain, sizeof(VtxPos) * CE_MAX_TERRAIN_VERTICES));
		terrainIndices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Terrain, sizeof(uint32_t) * CE_MAX_TERRAIN_INDICES));
	}

	void TerrainContainer::CLEAR()
	{
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Terrain, terrainVertices, sizeof(VtxPos) * CE_MAX_TERRAIN_VERTICES);
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Terrain, terrainIndices, sizeof(uint32_t) * CE_MAX_TERRAIN_INDICES);
	}

	TerrainContainer::~TerrainContainer()
	{
		if(terrainVertices != nullptr && terrainIndices != nullptr)
		{
			CLEAR();
		}
	}
}