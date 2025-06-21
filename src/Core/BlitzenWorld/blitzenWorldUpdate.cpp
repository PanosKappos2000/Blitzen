#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	inline static BLITZEN_SYSTEM_CONTEXT* S_P_CONTEXT = nullptr;

	static void DispatchUpdate(BlitzenWorldUpdate& update, BLITZEN_SYSTEM_CONTEXT& context)
	{
		
	}

	void UpdateLoop(BLITZEN_SYSTEM_CONTEXT& context)
	{
		S_P_CONTEXT = &context;

		// Nothing yet
	}

	void S_WORLD_UPDATE_RESIDENT_MOVED(uint32_t id)
	{
		S_P_CONTEXT->pComponents->m_movingResidents[id].isMoving = true;
	}
}