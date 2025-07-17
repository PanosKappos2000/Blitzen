#pragma once
#include "Renderer/Resources/blitShaderShared.h"
#include "Renderer/Resources/blitShaderResources.h"
#include "BlitCL/blitPfn.h"

namespace BlitzenCore
{
	constexpr uint32_t CE_MAX_FRAME_EVENTS = BLIT_MAX_WORLD_VARIABLE_COUNT;

	using FrameEventPfn = BlitCL::Pfn<void, BlitzenEngine::Resident, float>;

	struct FrameEvent
	{
		FrameEventPfn m_function;
		BlitzenEngine::Resident m_resident;
	};

	class FrameEventManager
	{
	public: 
		FrameEvent m_frameEvents[CE_MAX_FRAME_EVENTS];
		uint32_t m_frameEventCount{ 0 };

		void RegisterFrameEvent(BlitzenEngine::Resident resident, FrameEventPfn function);
	};
}