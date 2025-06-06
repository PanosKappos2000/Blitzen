#pragma once

#include "Core/blitzenEngine.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenIMGUI
{
	enum class DEAR_DASHER_RETURN_CODE : int32_t
	{
		SUCCESS = 1,
		IMGUI_HANDLE_CREATION_FAILED = -1,
		IMGUI_FUNCTION_FAILED = -2,

		VULKAN_HANDLE_CREATION_FAILED = -100,
		VULKAN_FUNCTION_FAILED = -200,
		VULKAN_COLOR_TARGET_NOT_FOUND = -300,

		UNKNOWN_RETURN_CODE = -1000,

		MAX_ENUMS = 0
	};

	inline const char* TranslateErrorCode(DEAR_DASHER_RETURN_CODE res)
	{
		switch (res)
		{
		case DEAR_DASHER_RETURN_CODE::IMGUI_HANDLE_CREATION_FAILED: return "IMGUI_ERROR res with: IMGUI_HANDLE_CREATION_FAILED";
		case DEAR_DASHER_RETURN_CODE::VULKAN_HANDLE_CREATION_FAILED: return "IMGUI_ERROR res with: VULKAN_HANDLE_CREATION_FAILED";
		case DEAR_DASHER_RETURN_CODE::UNKNOWN_RETURN_CODE: case DEAR_DASHER_RETURN_CODE::MAX_ENUMS: default: return "IMGUI_ERROR res with: UNKNOWN_RETURN_CODE";
		}
	}

	inline bool LOG_IMGUI_ERROR_MSG_AND_RETURN(DEAR_DASHER_RETURN_CODE res)
	{
		if (BlitzenCore::BLIT_CHECK_FAIL(res))
		{
			BLIT_ERROR(TranslateErrorCode(res));
			return false;
		}

		BLIT_WARN("Provided IMGUI return code was not error");
		return false;
	}
}