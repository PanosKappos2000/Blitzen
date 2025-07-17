#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "BlitCL/blitPfn.h"
#include "Renderer/Resources/blitShaderShared.h"
#include "Renderer/Entities/Residents/RenderObject/blitRender.h"
#include "BlitzenMathLibrary/blitMLLight.h"

namespace BlitzenEngine
{
	constexpr uint32_t GCCollisionGridExtent = BLIT_COLLISION_GRID_EXTENT;

	constexpr uint32_t GCCollisionCellExtent = BLIT_COLLISION_GRID_CELL_EXTENT;

	constexpr uint32_t GCCollsionFlatCount = BLIT_COLLISION_GRID_CELL_FLAT_COUNT;

	constexpr uint32_t CE_COLLISION_GRID_CELL_COUNT = BLIT_COLLISION_GRID_CELL_COUNT;
	constexpr uint32_t CE_OBJECTS_PER_COLLISION_GRID_CELL = BLIT_COLLISION_GRID_CELL_EXTENT;
	constexpr uint32_t CE_DYNAMIC_RESIDENTS_PER_COLLISION_GRID_CELL = BLIT_COLLISION_GRID_CELL_EXTENT;

	constexpr uint32_t CE_AVAILABLE_DYNAMIC_COLLIDER_SPACES = CE_DYNAMIC_RESIDENTS_PER_COLLISION_GRID_CELL * CE_COLLISION_GRID_CELL_COUNT;

	struct WVColliderResponse
	{
		BLITZEN_COLLISION_IDENTIFIER m_impactFlag{ BlitzenCollisionFlagsIgnore };
		uint64_t m_reactionFlags = 0;
	};

	class CollisionGrid
	{
	public:

		int32_t mOrigin;
		int32_t m_minBounds;
		int32_t m_maxBounds;

		GridCellOffsets mCellOffsets[CE_COLLISION_GRID_CELL_COUNT];
		uint32_t* m_colliderIndices{ nullptr };
		uint32_t m_colliderIndicesTotal{ 0 };
		uint32_t* WVColliderIndices{ nullptr };

		CollisionMessage* m_events{ nullptr };
		uint32_t m_count{ 0 };

		void DefineGrid(uint32_t origin);

		void CreateCells();

		// When a scene's static residents are set, this is called to place them in the correct cell.
		// This is supposed to be saved in a scene configuration file, and will not be called again after this scene is fully packaged.
		// Dynamic allocations fully allowed here. The transform array should be the full array, the offsets are placed inside
		void PlaceStatics(BlitzenEngine::MeshTransform* transformArr, uint32_t count);

		void PlaceDynamics(BlitzenEngine::WVTransform* transformArr, uint32_t count);

		void FindCollisionsNarrow(BoundingSphere* boundsArr);

		void BLITZEN_RESOLVE_RESIDENT_COLLISION_EVENTS(WVColliderResponse* colliderArr);

		void AllocStatics(uint32_t count);

		void AllocDynamicIndices();

		void ClearStaticIndices();

		void ClearDynamicIndices();

		~CollisionGrid();
	};

	bool CheckSphereCollision(const BoundingSphere& firstBounds, const BoundingSphere& secondBounds);
}