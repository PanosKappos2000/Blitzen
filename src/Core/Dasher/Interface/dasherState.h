#pragma once 
#include "Core/blitzenEngine.h"

namespace BlitzenCore
{
	enum class DASHER_STATE : int8_t
	{
		DASHER_EDITOR_FULL_BLIT_RENDERER_IDLE = 0,
		DASHER_EDITOR_DEBUG_RENDERER = 1,

		BLIT_RENDERER_FULL_DASHER_EDITOR_NO_JOIN = -1
	};
}