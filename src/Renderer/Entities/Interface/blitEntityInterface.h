#pragma once

#include "blitEntityManager.h"
#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenEngine
{
	enum class ENTITY_CREATE_RES : int8_t
	{
		SUCCESS = 0,

		ENTITIES_FULL = -1,
		DYNAMICALLY_UPDATED_ENTITIES_FULL = -2,
		DYNAMIC_ORIENTATION_FULL = -3,

		UNKNOWN = -126,
		MAX = -127
	};

	inline const char* GET_ENTITY_CREATE_RES_MSG(ENTITY_CREATE_RES res)
	{
		switch (res)
		{
		case ENTITY_CREATE_RES::SUCCESS: return "ENTITY_CREATE_RES_SUCCESS";
		case ENTITY_CREATE_RES::DYNAMICALLY_UPDATED_ENTITIES_FULL: return "ENTITY_CREATE_RES_DYNAMICALLY_UPDATED_ENTITIES_FULL";
		case ENTITY_CREATE_RES::ENTITIES_FULL: return "ENTITY_CREATE_RES_ENTITIES_FULL";
		case ENTITY_CREATE_RES::UNKNOWN: case ENTITY_CREATE_RES::MAX: default: return "ENTITY_CREATE_RES_UNKNOWN";
		}
	}

	inline bool LOG_ENTITY_CREATION_ERROR_MSG_AND_RETURN(ENTITY_CREATE_RES res)
	{
		if (BlitzenCore::BLIT_CHECK_FAIL(res))
		{
			BLIT_ERROR("Entity creation error result: %s", GET_ENTITY_CREATE_RES_MSG(res));
			return false;
		}

		BLIT_WARN("Entity creation, no error msg found. Error logging should not have been called");
		return false;
	}
}