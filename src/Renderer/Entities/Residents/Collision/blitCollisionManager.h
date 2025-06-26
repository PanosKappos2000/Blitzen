#pragma once
#include "blitCollision.h"

namespace BlitzenEngine
{
	enum class COLLISION_CREATE_RES : int64_t
	{
		SUCCESS = BlitzenCore::CE_BLITZEN_SUCCESS, 
		FATAL = BlitzenCore::CE_BLITZEN_FATAL, 
	};

	struct CollisionManager
	{
		Collision m_collisions[BlitzenCore::Ce_MaxWorldResidentCount];
		uint32_t m_colliderCount{ 0 };

		CollisionGrid* m_collisionGrids{ nullptr };
		uint32_t m_collisionGridCount{ 0 };
	};
}