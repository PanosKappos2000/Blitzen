#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Entities/Residents/blitResident.h"
#include "BlitCL/blitPfn.h"
#include "blitCollisionFlags.h"
#include "Renderer/HlslShaders/Headers/cpuShared.h"
#include "BlitzenMathLibrary/blitMLLight.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_COLLISION_GRID_EXTENT = 5'000;

	constexpr uint32_t CE_COLLISION_GRID_CELL_EXTENT = 100;

	constexpr uint32_t CE_COLLISION_GRID_CELL_FLAT_COUNT = CE_COLLISION_GRID_EXTENT / CE_COLLISION_GRID_CELL_EXTENT;

	constexpr uint32_t CE_COLLISION_GRID_CELL_COUNT = CE_COLLISION_GRID_CELL_FLAT_COUNT * CE_COLLISION_GRID_CELL_FLAT_COUNT * CE_COLLISION_GRID_CELL_FLAT_COUNT;
	constexpr uint32_t CE_GRID_CELL_COUNT_FLAT = (CE_COLLISION_GRID_EXTENT / CE_COLLISION_GRID_CELL_EXTENT);
	constexpr uint32_t CE_OBJECTS_PER_COLLISION_GRID_CELL = uint32_t(CE_COLLISION_GRID_CELL_EXTENT / 0.5f);
	constexpr uint32_t CE_DYNAMIC_RESIDENTS_PER_COLLISION_GRID_CELL = uint32_t(CE_COLLISION_GRID_CELL_EXTENT / 0.5f);

	constexpr uint32_t CE_AVAILABLE_COLLIDER_SPACES = CE_COLLISION_GRID_CELL_COUNT * CE_OBJECTS_PER_COLLISION_GRID_CELL;
	constexpr uint32_t CE_AVAILABLE_DYNAMIC_COLLIDER_SPACES = CE_DYNAMIC_RESIDENTS_PER_COLLISION_GRID_CELL * CE_COLLISION_GRID_CELL_COUNT;

	struct Collider
	{
		BLITZEN_COLLISION_IDENTIFIER m_impactFlag;
		uint64_t m_reactionFlags;
	};

	struct CollisionMessage
	{
		Resident m_impactingObject;
		Resident m_reactingResident;
	};

	struct CollisionGridCell
	{
		uint32_t colliderOffset;
		uint32_t colliderCount;
		uint32_t dynamicColliderOffset;
		uint32_t dynamicColliderCount;
	};

	class CollisionGrid
	{
	public:

		int32_t m_origin;
		int32_t m_minBounds;
		int32_t m_maxBounds;

		CollisionGridCell m_grids[CE_COLLISION_GRID_CELL_COUNT];
		uint32_t m_colliderIndices[CE_AVAILABLE_COLLIDER_SPACES];
		uint32_t m_dynamicColliderIndices[CE_AVAILABLE_DYNAMIC_COLLIDER_SPACES];

		CollisionMessage m_events[BlitzenCore::Ce_MaxWorldVariableCount];
		uint32_t m_count{ 0 };

		void DefineGrid(uint32_t origin);

		void CreateCells();

		void PlaceStatics(BlitzenEngine::RenderObject* renderArr, uint32_t count, BlitzenEngine::MeshTransform* transformArr);

		void FindCollisionsNarrow(BoundingSphere* boundsArr);

		void BLITZEN_RESOLVE_RESIDENT_COLLISION_EVENTS(Collider* colliderArr);
	};

	bool CheckSphereCollision(const BoundingSphere& firstBounds, const BoundingSphere& secondBounds);
}