#pragma once
#include "blitResident.h"
#include "RenderObject/blitRender.h"
#include "RenderObject/worldTransform.h"
#include "Collision/blitCollisionManager.h"
#include "Dynamic/blitMovingResident.h"

namespace BlitzenEngine
{
	using RESIDENT_CREATE_CONTEXT_FLAGS = int64_t;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_BASIC = 0;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_MOVING = 0x5;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_COLLISION = 0xA;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_WORLD_VARIABLE = 0xF;

	enum RESIDENT_CREATE_RES : int8_t
	{
		SUCCESS = 0,

		RENDER_OBJECT_CREATION_FAILED = -1,
		WORLD_TRANSFORM_CREATION_FAILED = -2,
		RESIDENT_CREATION_FAILED = -3,
		NO_WORLD_TRANSFORM_CONTEXT_GIVEN = -4,

		UNKNOWN = -10
	};

	inline const char* GET_RESIDENT_CREATE_RES_STRING(RESIDENT_CREATE_RES res)
	{
		switch (res)
		{
		case RESIDENT_CREATE_RES::SUCCESS: return "SUCCESS";
		case RENDER_OBJECT_CREATION_FAILED: return "RENDER_OBJECT_CREATION_FAILED";
		case WORLD_TRANSFORM_CREATION_FAILED: return "WORLD_TRANSFORM_CREATION_FAILED";
		case RESIDENT_CREATION_FAILED: return "RESIDENT_CREATION_FAILED";
		default: case RESIDENT_CREATE_RES::UNKNOWN: return "UNKNOWN";
		}
	}

	struct RESIDENT_CREATE_CONTEXT
	{
		RESIDENT_CREATE_CONTEXT_FLAGS m_flags{ RESIDENT_CREATE_BASIC };
		Mesh* m_pResource{ nullptr };
		TRANSFORM_CREATE_CONTEXT m_transformInfo{};
		BlitzenCore::FAT_BOOL m_isMoveable{ BlitzenCore::FAT_FALSE };
	};

	class WORLD_RESIDENTS
	{
	public:
		WVKEY m_worldVariableAccessors[BlitzenCore::Ce_MaxWorldVariableCount]{};
		uint32_t m_worldVariableCount{ 0 };
		Resident m_residents[BlitzenCore::Ce_MaxWorldResidentCount];
		uint32_t m_residentCount{ 0 };
		MovingResident m_movingResidents[BlitzenCore::Ce_MaxWorldMovingResidentCount];
		RenderContainer m_renders;
		WorldTransformContainer m_transforms;
		ColliderContainer m_colliders;

		RESIDENT_CREATE_RES AddResident(const RESIDENT_CREATE_CONTEXT& ctx);
	};

	void InitializeWorldResidentsPointer_STATIC_ACCESS(WORLD_RESIDENTS* ptr);

	RESIDENT_CREATE_RES AddResident_STATIC_ACCESS(const RESIDENT_CREATE_CONTEXT& ctx);
}