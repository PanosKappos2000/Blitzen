#pragma once

#include "Core/blitzenEngine.h"
#include "Core/Dasher/DearDasher/dearDasher.h"

namespace BlitzenCore
{
#if defined(DASHER_USE_DEAR)

	using Dasher = BlitzenIMGUI::DasherEditor;

#elif defined(DASHER_JOIN)

	static_assert(true);

#else

	using Dasher = uint8_t;

#endif
}