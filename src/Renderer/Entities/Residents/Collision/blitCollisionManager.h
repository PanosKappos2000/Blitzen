#pragma once 
#include "blitCollision.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_WORLD_BOUNDING_SPHERE_COUNT = BlitzenCore::Ce_MaxRenderObjectCount;

	struct ColliderContainer
	{
		Collision m_collisions[BlitzenCore::Ce_MaxWorldResidentCount];
		uint32_t m_colliderCount{ 0 };

		CollisionGrid m_collisionGrids[CE_MAX_WORLD_COLLISION_GRIDS];
		uint32_t m_collisionGridCount{ 0 };

		BoundingSphere m_boundingSpheres[CE_MAX_WORLD_BOUNDING_SPHERE_COUNT];
		uint32_t m_boundingSphereCount{ 0 };

		uint32_t AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform);
	};
}