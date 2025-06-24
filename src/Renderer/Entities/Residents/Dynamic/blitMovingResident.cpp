#include "blitMovingResident.h"
#include "Core/BlitzenWorld/blitzenWorldPrivate.h"

namespace BlitzenEngine
{
	void MovingResident::Rotate(BlitML::fRotation& rotation, float deltaTime)
	{
		m_pWorldTransform->eulerAngles += rotation;

		AddMovingResident_STATIC_ACCESS(this);
	}
}