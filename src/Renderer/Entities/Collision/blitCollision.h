#pragma once

#include "Renderer/Resources/renderingResourcesTypes.h"

namespace BlitzenEngine
{
	struct Collision
	{
		BlitML::mat4 m_boundingSphere;// Supposedly, this will already be transformed for the current frame
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