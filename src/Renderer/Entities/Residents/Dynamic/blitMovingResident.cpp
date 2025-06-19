#include "blitMovingResident.h"
#include "Core/BlitzenWorld/blitzenWorldPrivate.h"

namespace BlitzenEngine
{
	void RotateEntity(BlitML::fRotation& rotation, float deltaTime, uint32_t movingObjectID)
	{
		BlitzenWorld::S_WORLD_UPDATE_RESIDENT_MOVED(movingObjectID);
	}
}