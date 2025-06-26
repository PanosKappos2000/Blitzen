#pragma once
#include "Renderer/Entities/Residents/blitResident.h"

namespace BlitzenEngine
{
	class MovingResident
	{
	public:

		CPU_TRANSFORM* m_pWorldTransform;

		BlitzenCore::FAT_BOOL m_isBlocked{ BlitzenCore::FAT_FALSE };
	};

	MovingResident& GetMovingResident_STATIC_ACCESS(uint32_t residentID);

	MovingResident* RequestMovementComponent(uint32_t residentID);

	void RotateEntity(uint32_t residentID, const BlitML::fRotation& rotation, float deltaTime);
}