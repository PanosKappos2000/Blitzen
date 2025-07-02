#pragma once
#include "blitResident.h"
#include "RenderObject/blitRender.h"
#include "RenderObject/worldTransform.h"
#include "Collision/blitColliders.h"
#include "Dynamic/blitMovingResident.h"

namespace BlitzenEngine
{
	using RESIDENT_CREATE_CONTEXT_FLAGS = int64_t;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_BASIC = 0;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_MOVING = 0x5;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_COLLISION = 0xA;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_WORLD_VARIABLE = 0xF;

	enum class RESIDENT_CREATE_RES : int8_t
	{
		SUCCESS = 0,

		RENDER_OBJECT_CREATION_FAILED = -1,
		WORLD_TRANSFORM_CREATION_FAILED = -2,
		RESIDENT_CREATION_FAILED = -3,
		NO_WORLD_TRANSFORM_CONTEXT_GIVEN = -4,
		WORLD_VARIABLE_COUNT_EXCEEDED = -5,

		UNKNOWN = -10
	};

	inline const char* GET_RESIDENT_CREATE_RES_STRING(RESIDENT_CREATE_RES res)
	{
		switch (res)
		{
		case RESIDENT_CREATE_RES::SUCCESS: return "SUCCESS";
		case RESIDENT_CREATE_RES::RENDER_OBJECT_CREATION_FAILED: return "RENDER_OBJECT_CREATION_FAILED";
		case RESIDENT_CREATE_RES::WORLD_TRANSFORM_CREATION_FAILED: return "WORLD_TRANSFORM_CREATION_FAILED";
		case RESIDENT_CREATE_RES::RESIDENT_CREATION_FAILED: return "RESIDENT_CREATION_FAILED";
		case RESIDENT_CREATE_RES::WORLD_VARIABLE_COUNT_EXCEEDED: return "WORLD_VARIABLE_COUNT_EXCEEDED";
		default: case RESIDENT_CREATE_RES::UNKNOWN: return "UNKNOWN";
		}
	}

	struct RESIDENT_CREATE_CONTEXT
	{
		RESIDENT_CREATE_CONTEXT_FLAGS m_flags{ RESIDENT_CREATE_BASIC };
		uint32_t m_resourceID;
		TRANSFORM_CREATE_CONTEXT m_transformInfo{};
		BlitzenCore::FAT_BOOL m_isMoveable{ BlitzenCore::FAT_FALSE };
	};

	struct WORLD_VARIABLE_CREATE_CONTEXT
	{
		RESIDENT_CREATE_CONTEXT residentCtx{};
		uint32_t m_worldVariableID{ 0 };
	};

	class WORLD_RESIDENTS
	{
	public:
		WORLD_VARIABLE m_worldVariables[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		uint32_t m_worldVariableCount{ 0 };
		Resident m_residents[BLIT_MAX_WORLD_RESIDENTS];
		uint32_t m_residentCount{ 0 };
		MovingResident m_movingResidents[BLIT_MAX_WORLD_VARIABLE_COUNT];
		RenderContainer m_renders;
		WorldTransformContainer m_transforms;
		ColliderContainer m_colliders;

		RESIDENT_CREATE_RES AddResident(const RESIDENT_CREATE_CONTEXT& ctx);

		RESIDENT_CREATE_RES AddWorldVariable(const WORLD_VARIABLE_CREATE_CONTEXT& ctx);
	};

	void InitializeWorldResidentsPointer_STATIC_ACCESS(WORLD_RESIDENTS* ptr);

	RESIDENT_CREATE_RES AddResident_STATIC_ACCESS(const RESIDENT_CREATE_CONTEXT& ctx);
}