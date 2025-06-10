#include "blitzenWorldPrivate.h"

namespace BlitzenWorld
{
	void RenderLoop(BlitzenPrivateContext& context)
	{
		while (*context.pEngineState != BlitzenCore::EngineState::SHUTDOWN)
		{
			// TODO: CULL STATIC OBJECTS FIRST

			// TODO: Update view after waiting for camera

			// TODO: Wait for update loop
			for(uint32_t move = 0)
		}
	}
}