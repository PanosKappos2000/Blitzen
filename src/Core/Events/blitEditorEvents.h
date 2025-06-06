#pragma once

#include "Core/blitzenEngine.h"
#include "BlitCL/DynamicArray.h"
#include "BlitCL/blitPfn.h"
#include "Core/BlitzenWorld/blitzenWorldPrivate.h"

namespace BlitzenCore
{
	enum class EditorEventType : uint8_t
	{
		BUTTON_CLICK = 0,
		
		NO_EVENT = 20
	};

	using EditorCallback = BlitCL::Pfn<void, BlitzenWorld::BlitzenPrivateContext&>;

	struct EditorEventContext
	{
		EditorEventType m_events[Ce_EditorEventQueueSize];
		uint32_t m_eventId{ 0 };

		BlitCL::DynamicArray<EditorCallback> m_callbacks{};
	};
}