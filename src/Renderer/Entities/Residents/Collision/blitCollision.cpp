#include "blitColliders.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/blitMemory.h"

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
        m_origin = origin;
        m_minBounds =  origin - (CE_COLLISION_GRID_EXTENT / 2);
        m_maxBounds =  origin + (CE_COLLISION_GRID_EXTENT / 2);
    }

	void CollisionGrid::CreateCells()
	{
        for (uint32_t cellId = 0; cellId < CE_COLLISION_GRID_CELL_COUNT; ++cellId)
        {
            auto& gridCell = m_grids[cellId];

            gridCell.colliderOffset = cellId * CE_OBJECTS_PER_COLLISION_GRID_CELL;
            gridCell.colliderCount = 0;
            gridCell.dynamicColliderOffset = cellId * CE_DYNAMIC_RESIDENTS_PER_COLLISION_GRID_CELL;
            gridCell.dynamicColliderCount = 0;
        }
	}

    void CollisionGrid::PlaceStatics(BlitzenEngine::RenderObject* renderArr, uint32_t count, BlitzenEngine::MeshTransform* transformArr)
    {
        uint32_t outOfBounds = 0;
        uint32_t claustrophobic = 0;
        uint32_t badColliderIndex = 0;
        uint32_t badLogic = 0;
        // Loop through all static objects
        for (uint32_t i = 0; i < count; ++i)
        {
            BlitML::float3 position = transformArr[renderArr[i].transformId].pos;

            if ((int32_t)position.x > m_maxBounds || (int32_t)position.x < m_minBounds)
            {
                //BLIT_WARN("%s: Object out of collision grid bounds", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                outOfBounds++;
                continue;
            }
            if ((int32_t)position.y > m_maxBounds || (int32_t)position.y < m_minBounds)
            {
                //BLIT_WARN("%s: Object out of collision grid bounds", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                outOfBounds++;
                continue;
            }
            if ((int32_t)position.z > m_maxBounds || (int32_t)position.z < m_minBounds)
            {
                //BLIT_WARN("%s: Object out of collision grid bounds", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                outOfBounds++;
                continue;
            }

            position -= BlitML::float3(float(m_minBounds));

            BLIT_ASSERT_MESSAGE(position > 0.f || position < CE_COLLISION_GRID_EXTENT, "Something went wrong with collision grid calculations");

            // Calculates the grid cell the object belongs to based on position
            uint32_t cellPosX = (position.x / CE_COLLISION_GRID_CELL_EXTENT) < CE_COLLISION_GRID_CELL_FLAT_COUNT ? 
                uint32_t(position.x / CE_COLLISION_GRID_CELL_EXTENT): CE_COLLISION_GRID_CELL_FLAT_COUNT - 1;
            uint32_t cellPosY = (position.y / CE_COLLISION_GRID_CELL_EXTENT) < CE_COLLISION_GRID_CELL_FLAT_COUNT ? 
                uint32_t(position.y / CE_COLLISION_GRID_CELL_EXTENT): CE_COLLISION_GRID_CELL_FLAT_COUNT - 1;
            uint32_t cellPosZ = (position.z / CE_COLLISION_GRID_CELL_EXTENT) < CE_COLLISION_GRID_CELL_FLAT_COUNT ? 
                uint32_t(position.z / CE_COLLISION_GRID_CELL_EXTENT) : CE_COLLISION_GRID_CELL_FLAT_COUNT - 1;

            uint32_t cellIndex = cellPosX + cellPosY * CE_COLLISION_GRID_CELL_FLAT_COUNT + cellPosZ * CE_COLLISION_GRID_CELL_FLAT_COUNT * CE_COLLISION_GRID_CELL_FLAT_COUNT;
            if (cellIndex >= CE_COLLISION_GRID_CELL_COUNT)
            {
                badLogic++;
                continue;
            }

            auto& grid = m_grids[cellIndex];
            if (grid.colliderCount >= CE_OBJECTS_PER_COLLISION_GRID_CELL)
            {
                claustrophobic++;
                continue;
            }

            uint32_t colliderIndex = grid.colliderOffset + grid.colliderCount;
            if (colliderIndex >= CE_AVAILABLE_COLLIDER_SPACES)
            {
                badColliderIndex++;
                continue;
            }

            m_colliderIndices[colliderIndex] = i;
            m_grids[cellIndex].colliderCount++;
        }

        BLIT_INFO("%s: Ouf of collsion grid bounds object count: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, outOfBounds);
        BLIT_INFO("%s: Objects overflowing grid cell: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, claustrophobic);
        BLIT_INFO("%s: In bounds Objects hitting invalid grid cell: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, badColliderIndex);
        BLIT_INFO("%s: Bad grid index calculation: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, badLogic);
    }

    void ColliderContainer::AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform, uint32_t renderObjectID, bool isStatic)
    {
        auto& newcomer{ m_boundingSpheres[renderObjectID] };

        BlitzenCore::BlitMemCopy(&newcomer, pSphere, sizeof(BoundingSphere));

		if (isStatic)
		{
			newcomer.m_center = BlitML::RotateQuat(newcomer.m_center, transform.orientation) * transform.scale + transform.pos;
			newcomer.m_radius *= transform.scale;
		}
    }

    void CollisionGrid::FindCollisionsNarrow(BoundingSphere* boundsArr)
    {
        for (auto& gridCell : m_grids)
        {
            for (uint32_t dynamicID = gridCell.dynamicColliderOffset; dynamicID < gridCell.colliderCount; ++dynamicID)
            {
                uint32_t impactingBoundsID = m_dynamicColliderIndices[dynamicID];

                for (uint32_t staticID = gridCell.colliderOffset; staticID < gridCell.colliderCount; ++staticID)
                {
                    if (CheckSphereCollision(boundsArr[impactingBoundsID], boundsArr[m_colliderIndices[staticID]]))
                    {
                        // Create collision event message
                    }
                }

                for (uint32_t dynamicID = gridCell.dynamicColliderOffset; dynamicID < gridCell.dynamicColliderCount; ++dynamicID)
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

    void CollisionGrid::BLITZEN_RESOLVE_RESIDENT_COLLISION_EVENTS(Collider* colliderArr)
    {
        
    }
}