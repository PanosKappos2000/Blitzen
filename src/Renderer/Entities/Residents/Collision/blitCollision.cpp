#include "blitColliders.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "BlitzenMathLibrary/blitML.h"
#include "BlitCL/blitDynamicArr.h"

namespace BlitzenEngine
{
    void DispatchCollisionResolve(CollisionGrid* pGrid, const COLLISION_RESOLVE_CONTEXT& context)
    {
        if constexpr (!BLITGCBroadPhaseCollisionBumper && !BLITGCBroadPhaseCollisionBumper)
        {
            BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(context.WVTransformArr != 0);
            pGrid->PlaceDynamics(context.WVTransformArr, context.mTransformCount);
        }
    }

	bool CheckSphereCollision(const BoundingSphere& firstBounds, const BoundingSphere& secondBounds)
	{
		BlitML::vec3 delta = firstBounds.m_center - secondBounds.m_center;
		float distSq = BlitML::LengthSquared(delta);  // or delta.LengthSquared() if you have it
		float radiusSum = firstBounds.m_radius + secondBounds.m_radius;
		return distSq <= (radiusSum * radiusSum);
	}

    void CollisionGrid::DefineGrid(uint32_t origin)
    {
        mOrigin = origin;
        m_minBounds =  origin - (GCCollisionGridExtent / 2);
        m_maxBounds =  origin + (GCCollisionGridExtent / 2);
    }

	void CollisionGrid::CreateCells()
	{
        for (uint32_t cellId = 0; cellId < CE_COLLISION_GRID_CELL_COUNT; ++cellId)
        {
            mCellOffsets[cellId].staticCollidersOffset= 0;
            mCellOffsets[cellId].staticColliderCount = 0;
            mCellOffsets[cellId].dynamicCollidersOffset = 0;
            mCellOffsets[cellId].dynamicCollidersCount = 0;
        }
	}

    void CollisionGrid::PlaceStatics(BlitzenEngine::MeshTransform* transformArr, uint32_t count)
    {
        uint32_t outOfBounds = 0;
        uint32_t badLogic = 0;

        // Array of arrays.
        // Mother array represents all the cells
        // Children arrays represent the indices that each cell holds
        BlitCL::DynamicArray<BlitCL::DynamicArray<uint32_t>> colliderIndices{ CE_COLLISION_GRID_CELL_COUNT, {} };
        
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t IDX = i + BLIT_OPAQUE_STATIC_RENDER_OFFSET;
            BlitML::float3 position = transformArr[IDX].pos;

            // Residents that are not inside the grid, cannot be placed inside a cell.
            if ((int32_t)position.x > m_maxBounds || (int32_t)position.x < m_minBounds || (int32_t)position.z > m_maxBounds || (int32_t)position.z < m_minBounds)
            {
                outOfBounds++;
                continue;
            }

            // Moves world coordinates to [0, GRID_EXTENT]
            position -= BlitML::float3(float(m_minBounds));

            // Sanity. The first check should have removed such objects. If not, I am doing something wrong
            BLIT_ASSERT_MESSAGE(position > 0.f || position < GCCollisionGridExtent, "Something went wrong with collision grid calculations");

            // Creates an index for each axis based on resident position.
            // The id should not be above the cell's extent
            uint32_t cellPosX = BlitML::UMin(uint32_t(position.x / GCCollisionCellExtent), GCCollsionFlatCount - 1);
            uint32_t cellPosZ = BlitML::UMin(uint32_t(position.z / GCCollisionCellExtent), GCCollsionFlatCount - 1);

            // The flat index is retrieved by turning 2D to 1D
            // The xAxis is kept as it is and the Z is multiplied by the cell count on each axis (flat count)
            uint32_t cellIndex = cellPosX + cellPosZ * GCCollsionFlatCount;

            // Something wrong with indexing logic
            if (cellIndex >= CE_COLLISION_GRID_CELL_COUNT)
            {
                badLogic++;
                continue;
            }

            // Places index into collision cell
            colliderIndices[cellIndex].PushBack(IDX);
            mCellOffsets[cellIndex].staticColliderCount++;
        }

        uint32_t offset = 0;
        uint32_t cellIndex = 0;
        uint32_t finalCount = 0;
        // Flat array translator for the 2D dynamic array from before
        BlitCL::DynamicArray<uint32_t> inLineIndices{ BLIT_MAX_WORLD_RESIDENTS };

        for (auto& array : colliderIndices)
        {
            // For each array saves static offset and updates offset with the size of the array
            mCellOffsets[cellIndex++].staticCollidersOffset = offset;
            offset += uint32_t(array.GetSize()); 
        }

        // The offset will be the full count of all objects in the grid by the end of the above loop
        // It's used to allocate space for all indices
        AllocStatics(offset, inLineIndices.Data());

        // Copies the dynamic array to the raw pointer style array
        offset = 0;
        for (auto& array : colliderIndices)
        {
            BlitzenCore::MANUAL_COPY(&m_colliderIndices[offset], array.Data(), array.GetSize() * sizeof(uint32_t));
            offset += uint32_t(array.GetSize());
        }

        BLIT_ASSERT(mCellOffsets[CE_COLLISION_GRID_CELL_COUNT - 1].staticCollidersOffset + mCellOffsets[CE_COLLISION_GRID_CELL_COUNT - 1].staticColliderCount == m_colliderIndicesTotal);

        BLIT_INFO("%s: Ouf of collsion grid bounds object count: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, outOfBounds);
        BLIT_INFO("%s: Bad grid index calculation: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, badLogic);
    }

    void CollisionGrid::PlaceDynamics(BlitzenEngine::WVTransform* transformArr, uint32_t count)
    {
        // Resets all collider counts from previous frame
        for (auto& cellOffsets : mCellOffsets)
        {
            cellOffsets.dynamicCollidersCount = 0;
        }

        // Goes through all World Variables. Since only World Varialbes can move, they are the ones who will make collsion happen
        for (uint32_t IDX = 0; IDX < count; ++IDX)
        {
            auto& transform{ transformArr[IDX] };

            // Not making a check. I should make sure that dynamic transforms that are outside the grid get discarded or changed
            // CPU cannot afford these checks

            // This might change, but for now the grid does not have a position.
            // Instead every object is placed inside grid space by incrementing to grid bounds
            BlitML::float3 positionGridSpace = BlitML::float3(transform.position - (float)m_minBounds);

            // Gets the grid space position, finds its position on the x and z axis.
            // Finally, uses that position to create a flat index to find the correct cell for the resident
            uint32_t cellPosX = BlitML::UMin(uint32_t(positionGridSpace.x) / GCCollisionCellExtent, GCCollsionFlatCount);
            uint32_t cellPosZ = BlitML::UMin(uint32_t(positionGridSpace.z) / GCCollisionCellExtent, GCCollsionFlatCount);
            uint32_t cellIndex = cellPosX + cellPosZ * GCCollsionFlatCount;

            // Saves the cell Index for the final stage
            transform.targetIdx = cellIndex;
            // Saves the collider count for the next stage (offset stage)
            mCellOffsets[cellIndex].dynamicCollidersCount++;
        }
        
        // Keeps global offset and iterate over every cell
        uint32_t globalOffset = 0;
        for (auto& cellOffsets : mCellOffsets)
        {
            // Uses each cell's count to create an offset for the next cell.
            // Saves the previous offset created by the previous cell
            cellOffsets.dynamicCollidersOffset = globalOffset;
            globalOffset += cellOffsets.dynamicCollidersCount;

            // The collider count is reset so that collider indices can be placed in a flat array
            // The offset and the count will be used to access that array
            cellOffsets.dynamicCollidersCount = 0;
        }

        // This should always be true. The literal reason these steps are taken is that we do not overshoot and broad phase is precise in terms of count
        BLIT_RUNTIME_TEST_CHECK_ASSERT(globalOffset == BLIT_MAX_WORLD_VARIABLE_COUNT);

        // Goes over world variables one last time
        for (uint32_t IDX = 0; IDX < count; ++IDX)
        {
            // Retrieves the old cell index and accesses the resident's cell
            uint32_t cellIndex = transformArr[IDX].targetIdx;
            auto& cell = mCellOffsets[cellIndex];

            // Using collider offset and collider count, the resident's index is written to the flat array.
            WVColliderIndices[cell.dynamicCollidersOffset + cell.dynamicCollidersCount] = IDX;
            // Collider count gets incremented back to what was calculated earlier.
            cell.dynamicCollidersCount++;
        }
    }

    void CollisionGrid::AllocStatics(uint32_t count, uint32_t* data)
    {
        BLIT_ASSERT(m_colliderIndices == nullptr);

        m_colliderIndices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Entity, count * sizeof(uint32_t)));
        m_colliderIndicesTotal += count;
    }

    void ColliderContainer::AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform, uint32_t renderObjectID, RENDER_OBJECT_TYPE type)
    {
        auto& newcomer{ m_boundingSpheres[renderObjectID] };

        BlitzenCore::BlitMemCopy(&newcomer, pSphere, sizeof(BoundingSphere));

		if (type == RENDER_OBJECT_TYPE::OPAQUE_STATIC)
		{
			newcomer.m_center = BlitML::RotateQuat(newcomer.m_center, transform.orientation) * transform.scale + transform.pos;
			newcomer.m_radius *= transform.scale;
		}
    }

    bool ColliderContainer::LogResidentForCollision(Resident resident, SplitColliderDataPair& data, WVColliderResponse behavior)
    {
        if (MWorldColliderCount >= CE_MAX_WORLD_COLLIDER_COUNT)
        {
            BLIT_ERROR("%s: Max Collider count exceeded", BlitzenCore::GCCollisionSystemName)
            return false;
        }

        BlitzenCore::BlitMemCopy(&MColliderAMaxRad[resident], &data.AMaxRad, sizeof(BlitML::float4));
        BlitzenCore::BlitMemCopy(&MColliderBMinType[resident], &data.BMinType, sizeof(BlitML::float4));

        if (IS_RESIDENT_STATIC(resident))
        {
            if ((uint32_t)data.BMinType.data.w == BlitzenColliderTypeSphere)
            {
                // transform sphere
            }
            else if ((uint32_t)data.BMinType.data.w == BlitzenColliderTypeAABB)
            {
                // tranform AABB
            }
            else if ((uint32_t)data.BMinType.data.w == BlitzenColliderTypeCapsule)
            {
                // transform Capsule
            }
        }
        else
        {
            WVColliderHitData[resident] = behavior;
        }

        MWorldColliderCount++;

        return true;
    }

    void CollisionGrid::FindCollisionsNarrow(BoundingSphere* boundsArr)
    {
        
    }

    void CollisionGrid::BLITZEN_RESOLVE_RESIDENT_COLLISION_EVENTS(WVColliderResponse* colliderArr)
    {
        
    }

    void CollisionGrid::AllocDynamicIndices()
    {
        WVColliderIndices = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Entity, BLIT_MAX_WORLD_VARIABLE_COUNT * sizeof(uint32_t)));
    }

    CollisionGrid::~CollisionGrid()
    {
        if (m_colliderIndices)
        {
            BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Entity, m_colliderIndices, m_colliderIndicesTotal * sizeof(uint32_t));
        }

        if (WVColliderIndices)
        {
            BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Entity, WVColliderIndices, BLIT_MAX_WORLD_VARIABLE_COUNT * sizeof(uint32_t));
        }
    }
}