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
        for (uint32_t grid = 0; grid < CE_COLLISION_GRID_CELL_COUNT; ++grid)
        {
            m_gridsStaticCounts[grid] = 0;
            m_grids[grid].colliderOffset = grid * CE_OBJECTS_PER_COLLISION_GRID_CELL;
            m_grids[grid].colliderCount = 0;
        }
	}

    void CollisionGrid::PlaceStatics(BlitzenEngine::RenderObject* renderArr, uint32_t count, BlitzenEngine::MeshTransform* transformArr)
    {
        uint32_t outOfBounds = 0;
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

            BLIT_ASSERT_MESSAGE(position < 0.f || position > CE_COLLISION_GRID_EXTENT, "Something went wrong with collision grid calculations");

            // Calculates the grid cell the object belongs to based on position
            uint32_t cellPosX = uint32_t(position.x) / CE_COLLISION_GRID_CELL_EXTENT;
            uint32_t cellPosY = uint32_t(position.y) / CE_COLLISION_GRID_CELL_EXTENT;
            uint32_t cellPosZ = uint32_t(position.z) / CE_COLLISION_GRID_CELL_EXTENT;

            uint32_t cellIndex = cellPosX + cellPosY * CE_GRID_CELL_COUNT_FLAT + cellPosZ * CE_GRID_CELL_COUNT_FLAT * CE_GRID_CELL_COUNT_FLAT;

            BLIT_ASSERT_MESSAGE(cellIndex < CE_COLLISION_GRID_CELL_COUNT, "Collision grid cell index out of bounds");

            uint32_t colliderIndex = m_grids[cellIndex].colliderOffset + m_gridsStaticCounts[cellIndex];

            BLIT_ASSERT_MESSAGE(cellIndex < CE_AVAILABLE_COLLIDER_SPACES, "Collider index out of bounds");

            BLIT_ASSERT_MESSAGE(m_colliderIndices[colliderIndex] == 0, "Multiple colliders in the same offset");
            m_colliderIndices[colliderIndex] = i;

            // Increment the static count for the grid cell
            m_gridsStaticCounts[cellIndex]++;
            m_grids[cellIndex].colliderCount++;

            BLIT_ASSERT_MESSAGE(m_grids[cellIndex].colliderCount < CE_OBJECTS_PER_COLLISION_GRID_CELL, "Too many objects in one cell");
        }

        BLIT_INFO("%s: Ouf of collsion grid bounds object count: %u", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, outOfBounds);
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

    void CollisionGrid::BLITZEN_RESOLVE_RESIDENT_COLLISION_EVENTS(Collider* colliderArr)
    {
        for (uint32_t msg = 0; msg < m_count; ++msg)
        {
            auto message = m_events[msg];
            auto& impactData = colliderArr[message.m_impactingObject];
            auto& receiveData = colliderArr[message.m_reactingResident];

            switch (impactData.m_collisionIdentifier)
            {
            case BLITZEN_COLLISION_IDENTIFIER::BLITZEN_COLLISION_FLAGS_BLOCK:
            {
                //BRRCE_BlitzenCollisionFlagsBlock();
                break;
            }
            case BLITZEN_COLLISION_FLAGS_WORLD_VARIABLE:
            {
                switch (impactData.m_worldVariable.m_worldVariableID)
                {
                default:
                {
                    break;
                }
                }
                break;
            }
            default:
            {
                break;
            }
            }

        }
    }
}