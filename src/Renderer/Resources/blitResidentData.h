#pragma once
#include "BlitCL/blitPfn.h"
#include "blitShaderResources.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_FRAME_EVENTS = BLIT_MAX_WORLD_VARIABLE_COUNT;

	using ResidentFrameEventPfn = BlitCL::Pfn<void, BlitzenEngine::Resident, float>;
}