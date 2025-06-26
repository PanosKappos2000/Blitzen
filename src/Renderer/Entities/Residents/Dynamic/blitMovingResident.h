#pragma once
#include "Renderer/Entities/Residents/blitResident.h"

namespace BlitzenEngine
{
	class MovingResident
	{
	public:

		CPU_TRANSFORM* m_pWorldTransform;

		BlitzenCore::FAT_BOOL m_isBlocked{ BlitzenCore::FAT_FALSE };

		void Rotate(BlitML::fRotation& rotation, float deltaTime);
	};

	MovingResident& GetMovingResident_STATIC_ACCESS(uint32_t residentID);

	MovingResident* RequestMovementComponent();
}