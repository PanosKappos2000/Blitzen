#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Entities/Residents/blitResident.h"
#include "BlitCL/blitPfn.h"

namespace BlitzenEngine
{
	using BLITZEN_COLLISION_FLAGS = int32_t;
	constexpr BLITZEN_COLLISION_FLAGS BLITCF_BLOCK_MOVEMENT = 0;

	struct Collision
	{
		BLITZEN_COLLISION_FLAGS m_flags;
		BLITZEN_COLLISION_FLAGS m_reactions;
		uint32_t m_engineCompAccess;
		uint32_t m_clientCompAccess;
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

	};

	bool CheckSphereCollision(const BoundingSphere& firstBounds, const BoundingSphere& secondBounds);
}