#include "blitTerrain.h"
#include "Core/blitMemory.h"
#include "Core/DbLog/blitAssert.h"
#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenEngine
{
	inline TerrainContainer* GSTrerrainContainer = nullptr;

	void ResidentSnapDown(BlitzenEngine::MeshTransform& transform, float radius)
	{
		if (transform.pos.x >= 0 && transform.pos.x < BLIT_TERRAIN_GRID_SIZE_TEMP && transform.pos.z >= 0 && transform.pos.z < BLIT_TERRAIN_GRID_SIZE_TEMP)
		{
			int32_t gridX = (int32_t)BlitML::FFloor(transform.pos.x);
			int32_t gridZ = (int32_t)BlitML::FFloor(transform.pos.z);
			int heightDataIndex = gridX + gridZ * BLIT_TERRAIN_GRID_SIZE_TEMP;
			transform.pos.y = GSTrerrainContainer->m_heightBufferData[heightDataIndex] + radius * transform.scale;
		}
		else
		{
			transform.pos.y = BLIT_TERRAIN_HEIGHT_TEST_VALUE + radius * transform.scale;
		}
	}

	bool TerrainContainer::AppendVertices(BlitzenEngine::VtxPos* vertices, uint32_t vertexCount)
	{
		if(vertexCount == 0 || vertices == nullptr || terrainVertexCount + vertexCount > CE_MAX_TERRAIN_VERTICES || terrainVertices == nullptr)
		{
			return false;
		}

		BlitzenCore::MANUAL_COPY(&terrainVertices[terrainVertexCount], vertices, sizeof(VtxPos) * vertexCount);
		terrainVertexCount += vertexCount;
		return true;
	}

	bool TerrainContainer::AppendIndices(uint32_t* indices, uint32_t indexCount)
	{
		if (indexCount == 0 || indices == nullptr || terrainIndexCount + indexCount > CE_MAX_TERRAIN_INDICES || terrainIndices == nullptr)
		{
			return false;
		}

		BlitzenCore::MANUAL_COPY(&terrainIndices[terrainIndexCount], indices, sizeof(uint32_t) * indexCount);
		terrainIndexCount += indexCount;
		return true;
	}

	bool TerrainContainer::AppendHeightData(float* heightArr, uint32_t dataCount)
	{
		if (dataCount == 0 || heightArr == nullptr || m_heightDataCount + dataCount > BLIT_MAX_HEIGHT_MAP_DATA_COUNT || m_heightBufferData == nullptr)
		{
			return false;
		}

		BlitzenCore::MANUAL_COPY(&m_heightBufferData[m_heightDataCount], heightArr, sizeof(float) * dataCount);
		m_heightDataCount += dataCount;
		return true;
	}

	void TerrainContainer::ALLOC()
	{
		terrainVertices = reinterpret_cast<VtxPos*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Terrain, sizeof(VtxPos) * CE_MAX_TERRAIN_VERTICES));
		terrainIndices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Terrain, sizeof(uint32_t) * CE_MAX_TERRAIN_INDICES));
		m_heightBufferData = reinterpret_cast<float*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Terrain, sizeof(float) * BLIT_MAX_HEIGHT_MAP_DATA_COUNT));
	}

	void TerrainContainer::CLEAR()
	{
		if (terrainVertices != nullptr)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Terrain, terrainVertices, sizeof(VtxPos) * CE_MAX_TERRAIN_VERTICES);
		}
		if (terrainIndices != nullptr)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Terrain, terrainIndices, sizeof(uint32_t) * CE_MAX_TERRAIN_INDICES);
		}
		if (m_heightBufferData != nullptr)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Terrain, m_heightBufferData, sizeof(float) * BLIT_MAX_HEIGHT_MAP_DATA_COUNT);
		}
	}

	TerrainContainer::~TerrainContainer()
	{
		CLEAR();
	}

	void InitializeTerrainContainerPtr(TerrainContainer* ptr)
	{
		BLIT_ASSERT_MESSAGE(GSTrerrainContainer == nullptr, "Tried to reinitialize global terrain pointer");

		GSTrerrainContainer = ptr;
	}
}