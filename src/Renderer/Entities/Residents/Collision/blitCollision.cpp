#include "blitColliders.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "BlitzenMathLibrary/blitML.h"
#include "BlitCL/blitDynamicArr.h"

namespace BlitzenEngine
{
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
            m_cellStaticOffsets[cellId].colliderOffset = 0;
            m_cellStaticOffsets[cellId].colliderCount = 0;
            m_cellDynamicOffsets[cellId].colliderOffset = 0;
            m_cellDynamicOffsets[cellId].colliderCount = 0;
        }
	}

    void CollisionGrid::PlaceStatics(BlitzenEngine::RenderObject* renderArr, uint32_t count, BlitzenEngine::MeshTransform* transformArr)
    {
        uint32_t outOfBounds = 0;
        uint32_t badLogic = 0;

        // Array of arrays.
        // Mother array represents all the cells
        // Children arrays represent the indices that each cell holds
        BlitCL::DynamicArray<BlitCL::DynamicArray<uint32_t>> colliderIndices{ CE_COLLISION_GRID_CELL_COUNT, {} };
        
        for (uint32_t i = 0; i < count; ++i)
        {
            BlitML::float3 position = transformArr[renderArr[i].transformId].pos;

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
            uint32_t cellPosX = BlitML::UMin(uint32_t(position.x / GCCollisionCellExtent), CE_COLLISION_GRID_CELL_FLAT_COUNT - 1);
            uint32_t cellPosZ = BlitML::UMin(uint32_t(position.z / GCCollisionCellExtent), CE_COLLISION_GRID_CELL_FLAT_COUNT - 1);

            // The flat index is retrieved by turning 2D to 1D
            // The xAxis is kept as it is and the Z is multiplied by the cell count on each axis (flat count)
            uint32_t cellIndex = cellPosX + cellPosZ * CE_COLLISION_GRID_CELL_FLAT_COUNT;

            // Something wrong with indexing logic
            if (cellIndex >= CE_COLLISION_GRID_CELL_COUNT)
            {
                badLogic++;
                continue;
            }

            // Places index into collision cell
            colliderIndices[cellIndex].PushBack(i);
            m_cellStaticOffsets[cellIndex].colliderCount++;
        }

        uint32_t offset = 0;
        uint32_t cellIndex = 0;
        uint32_t finalCount = 0;
        // Flat array translator for the 2D dynamic array from before
        BlitCL::DynamicArray<uint32_t> inLineIndices{ BLIT_MAX_WORLD_RESIDENTS };

        for (auto& array : colliderIndices)
        {
            // For each array saves static offset and updates offset with the size of the array
            m_cellStaticOffsets[cellIndex++].colliderOffset = offset;
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

        BLIT_ASSERT(m_cellStaticOffsets[CE_COLLISION_GRID_CELL_COUNT - 1].colliderOffset + m_cellStaticOffsets[CE_COLLISION_GRID_CELL_COUNT - 1].colliderCount == m_colliderIndicesTotal);

        BLIT_INFO("%s: Ouf of collsion grid bounds object count: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, outOfBounds);
        BLIT_INFO("%s: Bad grid index calculation: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, badLogic);
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
        for (uint32_t IDX = 0; IDX < CE_COLLISION_GRID_CELL_COUNT; IDX++)
        {
            auto& dynamics = m_cellDynamicOffsets[IDX];
            auto& statics = m_cellStaticOffsets[IDX];

            for (uint32_t dynamicID = dynamics.colliderOffset; dynamicID < dynamics.colliderCount; ++dynamicID)
            {
                uint32_t impactingBoundsID = m_dynamicColliderIndices[dynamicID];

                for (uint32_t staticID = statics.colliderOffset; staticID < statics.colliderCount; ++staticID)
                {
                    if (CheckSphereCollision(boundsArr[impactingBoundsID], boundsArr[m_colliderIndices[staticID]]))
                    {
                        // Create collision event message
                    }
                }

                for (uint32_t dynamicID = dynamics.colliderOffset; dynamicID < dynamics.colliderCount; ++dynamicID)
                {
                    uint32_t receiverBoundsID = m_dynamicColliderIndices[dynamicID];
                    if (impactingBoundsID == receiverBoundsID)
                    {
                        continue;
                    }

                    if (CheckSphereCollision(boundsArr[impactingBoundsID], boundsArr[receiverBoundsID]))
                    {
                        // CreateCollision message
                    }
                }
            }
        }
    }

    void CollisionGrid::BLITZEN_RESOLVE_RESIDENT_COLLISION_EVENTS(WVColliderResponse* colliderArr)
    {
        
    }

    CollisionGrid::~CollisionGrid()
    {
        if (m_colliderIndices)
        {
            BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Entity, m_colliderIndices, m_colliderIndicesTotal * sizeof(uint32_t));
        }
    }
}