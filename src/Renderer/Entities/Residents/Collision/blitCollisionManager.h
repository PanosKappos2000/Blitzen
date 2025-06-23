#pragma once 
#include "blitCollision.h"
#include "Renderer/HlslShaders/Headers/cpuShared.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_WORLD_BOUNDING_SPHERE_COUNT = BLIT_MAX_WORLD_RENDERS;

	class ColliderContainer
	{
	public:

		Collision m_collisions[BlitzenCore::Ce_MaxWorldResidentCount];
		uint32_t m_colliderCount{ 0 };

		CollisionGrid m_collisionGrids[CE_MAX_WORLD_COLLISION_GRIDS];
		uint32_t m_collisionGridCount{ 0 };

		BoundingSphere m_boundingSpheres[CE_MAX_WORLD_BOUNDING_SPHERE_COUNT];

		void AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform, uint32_t renderObjectID, bool isStatic);
	};
}