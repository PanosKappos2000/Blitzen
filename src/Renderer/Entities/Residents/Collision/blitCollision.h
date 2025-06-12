#pragma once

#include "Renderer/Resources/renderingResourcesTypes.h"

namespace BlitzenEngine
{
	using BLITZEN_COLLISION_FLAGS = uint64_t;

	using COLLISION_PFN = BlitCL::Pfn<void, void*, BLITZEN_COLLISION_FLAGS, void*, BLITZEN_COLLISION_FLAGS>;

	struct Collision
	{
		BoundingSphere* m_pSphere;

		void* pWVDATA{ nullptr };

		BLITZEN_COLLISION_FLAGS m_flags;

		COLLISION_PFN BEHAVIOUR_blitpfn;
	};

	struct CollisionEvent
	{
		Collision* pSender{ nullptr };

		Collision* pReceiver{ nullptr };
	};

	enum class COLLISION_GRID_RES : int8_t
	{
		SUCCESS = 0,

		GRID_FOUND = 1,

		OBJECT_OUT_OF_WORLD_BOUNDS = -1,

		MAX_RES = -100
	};

	struct CollisionGrid
	{
		Collision* m_staticCollisions[BlitzenCore::Ce_MaxCollisionsInGrid];
		uint32_t m_staticCollisionCount{ 0 };
		uint32_t m_dynamicCollisionCount{ 0 };
		uint32_t m_dynamicCollisionOffset{ BlitzenCore::Ce_CollisionGridDynamicOffset };
	};
}