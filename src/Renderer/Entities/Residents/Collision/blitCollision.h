#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Entities/Residents/blitResident.h"
#include "BlitCL/blitPfn.h"

namespace BlitzenEngine
{
	constexpr float CE_COLLISION_GRID_WIDTH = 1'000.f;
	constexpr float CE_COLLISION_GRID_HEIGHT = 1'000.f;
	constexpr float CE_COLLISION_GRID_DEPTH = 1'000.f;
	constexpr size_t CE_COLLISION_GRID_VOLUME = size_t(CE_COLLISION_GRID_WIDTH) * size_t(CE_COLLISION_GRID_HEIGHT) * size_t(CE_COLLISION_GRID_DEPTH);

	constexpr float CE_COLLSION_GRID_CELL_WIDTH = 100.f;
	constexpr float CE_COLLISION_GRID_CELL_HEIGHT = 100.f;
	constexpr float CE_COLLISION_GRID_CELL_DEPTH = 100.f;
	constexpr size_t CE_COLLISION_GRID_CELL_VOLUME = size_t(CE_COLLSION_GRID_CELL_WIDTH) * size_t(CE_COLLISION_GRID_CELL_HEIGHT) * size_t(CE_COLLISION_GRID_CELL_DEPTH);

	constexpr size_t CE_COLLISION_GRID_CELL_COUNT = (size_t(CE_COLLISION_GRID_WIDTH) / size_t(CE_COLLSION_GRID_CELL_WIDTH)) *
													(size_t(CE_COLLISION_GRID_HEIGHT) / size_t(CE_COLLISION_GRID_CELL_HEIGHT)) *
													(size_t(CE_COLLISION_GRID_DEPTH) / size_t(CE_COLLISION_GRID_CELL_DEPTH));
	constexpr size_t CE_GENEROUS_OBJECT_SIZE = 1.f;
	constexpr size_t CE_OBJECTS_PER_COLLISION_GRID_CELL = CE_COLLISION_GRID_CELL_COUNT / CE_GENEROUS_OBJECT_SIZE;

	enum class BLITZEN_COLLISION_FLAGS_BITS : uint32_t
	{
		BLOCK = 0x0,
	};

	struct Collider
	{
		uint32_t m_impactFlags;
		uint32_t m_reactionFlags;
		uint32_t m_engineCompAccess;
		uint32_t m_clientCompAccess;
	};

	struct CollisionMessage
	{
		uint32_t m_impactingObject;
		uint32_t m_receiverObject;
	};

	struct CollisionSpecializedAABB
	{
		BlitML::float3 m_minBounds;
		BlitML::float3 m_maxBounds;
		uint32_t colliderOffset;
		uint32_t colliderCount;
	};
	using CollsionGridCell = CollisionSpecializedAABB;

	class CollisionGrid
	{
	public:

		CollsionGridCell m_grids[CE_COLLISION_GRID_CELL_COUNT];
		uint32_t m_colliderIndices[CE_COLLISION_GRID_CELL_COUNT * CE_OBJECTS_PER_COLLISION_GRID_CELL];
		uint32_t m_gridsStaticCounts[CE_COLLISION_GRID_CELL_COUNT];

		void CreateCells();

		void PlaceStatics(BlitzenEngine::MeshTransform* pTransform);
	};

	bool CheckSphereCollision(const BoundingSphere& firstBounds, const BoundingSphere& secondBounds);
}