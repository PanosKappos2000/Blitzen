#pragma once

#include "Core/blitzenEngine.h"
#include "Core/Dasher/DearDasher/dearDasher.h"

namespace BlitzenCore
{
#if defined(DASHER_USE_DEAR)

	using Dasher = BlitzenIMGUI::DasherUI;

#else

	static_assert(true);

#endif
}