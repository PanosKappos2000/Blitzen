#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Entities/Residents/blitResident.h"
#include "BlitCL/blitPfn.h"
#include "blitCollisionFlags.h"
#include "Renderer/Resources/blitShaderShared.h"
#include "Renderer/Entities/Residents/RenderObject/blitRender.h"
#include "BlitzenMathLibrary/blitMLLight.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_COLLISION_GRID_EXTENT = BLIT_COLLISION_GRID_EXTENT;

	constexpr uint32_t CE_COLLISION_GRID_CELL_EXTENT = BLIT_COLLISION_GRID_CELL_EXTENT;

	constexpr uint32_t CE_COLLISION_GRID_CELL_FLAT_COUNT = BLIT_COLLISION_GRID_CELL_FLAT_COUNT;

	constexpr uint32_t CE_COLLISION_GRID_CELL_COUNT = BLIT_COLLISION_GRID_CELL_COUNT;
	constexpr uint32_t CE_OBJECTS_PER_COLLISION_GRID_CELL = BLIT_COLLISION_GRID_CELL_EXTENT;
	constexpr uint32_t CE_DYNAMIC_RESIDENTS_PER_COLLISION_GRID_CELL = BLIT_COLLISION_GRID_CELL_EXTENT;

	constexpr uint32_t CE_AVAILABLE_DYNAMIC_COLLIDER_SPACES = CE_DYNAMIC_RESIDENTS_PER_COLLISION_GRID_CELL * CE_COLLISION_GRID_CELL_COUNT;

	struct WVColliderResponse
	{
		BLITZEN_COLLISION_IDENTIFIER m_impactFlag;
		uint64_t m_reactionFlags;
	};

	struct CollisionMessage
	{
		Resident m_impactingObject;
		Resident m_reactingResident;
	};

	class CollisionGrid
	{
	public:

		int32_t m_origin;
		int32_t m_minBounds;
		int32_t m_maxBounds;

		GridCellOffsets m_cellStaticOffsets[CE_COLLISION_GRID_CELL_COUNT];
		GridCellOffsets m_cellDynamicOffsets[CE_COLLISION_GRID_CELL_COUNT];
		uint32_t* m_colliderIndices{ nullptr };
		uint32_t m_colliderIndicesTotal{ 0 };
		uint32_t m_dynamicColliderIndices[CE_AVAILABLE_DYNAMIC_COLLIDER_SPACES];

		CollisionMessage m_events[BLIT_MAX_WORLD_VARIABLE_COUNT];
		uint32_t m_count{ 0 };

		void DefineGrid(uint32_t origin);

		void CreateCells();

		// When a scene's static residents are set, this is called to place them in the correct cell.
		// This is supposed to be saved in a scene configuration file, and will not be called again after this scene is fully packaged.
		// Dynamic allocations fully allowed here.
		void PlaceStatics(BlitzenEngine::RenderObject* renderArr, uint32_t count, BlitzenEngine::MeshTransform* transformArr);

		// Helper function for PlaceStatics. 
		// Once the static object count is known, it allocates a pool of indices
		void AllocStatics(uint32_t count, uint32_t* data);

		void FindCollisionsNarrow(BoundingSphere* boundsArr);

		void BLITZEN_RESOLVE_RESIDENT_COLLISION_EVENTS(WVColliderResponse* colliderArr);

		~CollisionGrid();
	};

	bool CheckSphereCollision(const BoundingSphere& firstBounds, const BoundingSphere& secondBounds);
}