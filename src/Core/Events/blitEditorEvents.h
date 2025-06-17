#pragma once
#include "Core/blitzenEngine.h"
#include "BlitCL/blitPfn.h"

namespace BlitzenCore
{
	enum class EditorEventType : uint8_t
	{
		BUTTON_CLICK = 0,
		NO_EVENT = 20
	};

	struct EditorEvent
	{
		EditorEventType m_type{ EditorEventType::NO_EVENT };
		uint8_t m_eventTypeID;
	};

	struct EditorEventContext
	{
		EditorEvent m_events[Ce_EditorEventQueueSize];
		uint32_t m_currentID{ 0 };
	};
}