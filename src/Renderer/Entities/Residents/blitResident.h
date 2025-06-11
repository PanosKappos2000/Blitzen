#pragma once

#include "Renderer/Resources/renderingResourcesTypes.h"

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

	struct Resident
	{
		BoundingSphere* m_pCollision{ nullptr };
		RenderObject* m_pRenderObject{ nullptr };
		uint32_t renderObjectCount;
		BlitzenEngine::MeshTransform* pTransform{ nullptr };

		RESIDENT_CREATE_RES CreateResident();
	};
}