#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Entities/Residents/blitResident.h"
#include "BlitCL/blitPfn.h"

namespace BlitzenEngine
{
	using BLITZEN_COLLISION_FLAGS = uint64_t;

	using COLLISION_PFN = BlitCL::Pfn<void, void*, BLITZEN_COLLISION_FLAGS, void*, BLITZEN_COLLISION_FLAGS>;

	constexpr uint32_t CE_MAX_COLLISION_EVENTS = 1'000;
	constexpr uint32_t CE_MAX_COLLISIONS_IN_GRID = 100;
	constexpr uint32_t CE_MAX_WORLD_COLLISION_GRIDS = 1'000'000;

	struct Collision
	{
		BoundingSphere* m_pSphere;

		Resident* pResident{ nullptr };

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
		uint32_t m_totalColliderCount{ 0 };
		Collision* m_pStatics[CE_MAX_COLLISIONS_IN_GRID]{ nullptr };
		uint32_t m_staticCount{ 0 };
		Collision* m_pMoving[CE_MAX_COLLISIONS_IN_GRID]{ nullptr };
		uint32_t m_dynamicCount{ 0 };

		// Some sort of transform data as well I am guessing
	};

	bool CheckSphereCollision(const BoundingSphere& firstBounds, const BoundingSphere& secondBounds);
}