#pragma once

#include "blitResident.h"
#include "blitWv.h"
#include "RenderObject/blitRender.h"
#include "RenderObject/worldTransform.h"

namespace BlitzenEngine
{
	using RESIDENT_CREATE_CONTEXT_FLAGS = int64_t;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_MOVING = 0x5;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_COLLISION = 0xA;

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

	inline bool LOG_RESIDENT_ERROR_MSG_AND_RETURN(RESIDENT_CREATE_RES res)
	{
		if (BlitzenCore::BLIT_CHECK_FAIL(res))
		{
			BLIT_ERROR(GET_RESIDENT_CREATE_RES_STRING(res));
			return false;
		}

		BLIT_WARN("No resident create error found");
		return false;
	}

	struct RESIDENT_CREATE_CONTEXT
	{
		RESIDENT_CREATE_CONTEXT_FLAGS m_flags;
		Mesh* m_pResource{ nullptr };
		TRANSFORM_CREATE_CONTEXT m_transformInfo{};
	};

	struct WORLD_RESIDENTS
	{
		WV m_worldVariableAccessors[BlitzenCore::Ce_MaxWorldVariableCount];
		WVHOST m_worldVariableHost;

		Resident m_residents[BlitzenCore::Ce_MaxWorldResidentCount];
		uint32_t m_residentCount{ 0 };

		RenderContainer m_renders;

		WorldTransformContainer m_transforms;

		RESIDENT_CREATE_RES AddResident(const RESIDENT_CREATE_CONTEXT& ctx);
	};
}