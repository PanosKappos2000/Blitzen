#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	inline static BlitzenPrivateContext* S_P_CONTEXT = nullptr;

	static void DispatchUpdate(BlitzenWorldUpdate& update, BlitzenPrivateContext& context)
	{
		
	}

	void UpdateLoop(BlitzenPrivateContext& context)
	{
		S_P_CONTEXT = &context;

		// Nothing yet
	}

	void S_WORLD_UPDATE_RESIDENT_MOVED(uint32_t id)
	{
		S_P_CONTEXT->pComponents->m_movingResidents[id].isMoving = true;
	}
}