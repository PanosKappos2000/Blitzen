#pragma once
#include "Renderer/Entities/Residents/blitWV.h"
#include "BlitCL/blitPfn.h"

namespace BlitzenCore
{
	using FrameEventPfn = BlitCL::Pfn<void, BlitzenEngine::WORLD_VARIABLE, float>;

	struct FrameEvent
	{
		FrameEventPfn m_function;
		BlitzenEngine::WORLD_VARIABLE m_worldVariableArg;
	};

	class FrameEventManager
	{
	public: 
		FrameEvent m_frameEvents[CE_MAX_FRAME_EVENTS];
		uint32_t m_frameEventCount{ 0 };

		void RegisterFrameEvent(BlitzenEngine::WORLD_VARIABLE worldVariable, FrameEventPfn function);
	};
}