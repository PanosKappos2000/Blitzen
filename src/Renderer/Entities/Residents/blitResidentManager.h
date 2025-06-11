#pragma once

#include "blitResident.h"
#include "blitWv.h"
#include "RenderObject/blitRender.h"
#include "RenderObject/worldTransform.h"

namespace BlitzenEngine
{
	using RESIDENT_CREATE_FLAGS = int64_t;

	enum RESIDENT_CREATE_RES : int8_t
	{
		SUCCESS = 0,

		UNKNOWN = -10
	};

	inline const char* RETURN_RESIDENT_CREATE_RES_STRING(RESIDENT_CREATE_RES res)
	{
		switch (res)
		{
		case RESIDENT_CREATE_RES::SUCCESS: return "RESIDENT_CREATE_RES->SUCCESS";
		default: case RESIDENT_CREATE_RES::UNKNOWN: return "RESIDENT_CREATE_RES->UNKNOWN";
		}
	}

	inline bool LOG_RESIDENT_ERROR_MSG_AND_RETURN(RESIDENT_CREATE_RES res)
	{
		if (BlitzenCore::BLIT_CHECK_FAIL(res))
		{
			BLIT_ERROR(RETURN_RESIDENT_CREATE_RES_STRING(res));
			return false;
		}

		BLIT_WARN("No resident create error found");
		return false;
	}

	struct RESIDENT_CREATE_CONTEXT
	{
		RESIDENT_CREATE_FLAGS m_flags;

		Mesh* m_pResource;
		BlitzenEngine::MeshTransform* p_mTransform{ nullptr };
	};

	struct WORLD_RESIDENTS
	{
		BlitCL::BlitStack<WV, BlitzenCore::Ce_MaxWorldVariableCount> m_worldVariableAccessors;
		WVHOST m_worldVariableHost;

		Resident m_resident;

		RenderContainer m_renders;

		WorldTransformContainer m_transform;

		RESIDENT_CREATE_RES CreateResident(RESIDENT_CREATE_CONTEXT ctx);
	};
}