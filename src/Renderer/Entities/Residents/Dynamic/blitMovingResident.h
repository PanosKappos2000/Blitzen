#pragma once
#include "Renderer/Entities/Residents/blitResident.h"

namespace BlitzenEngine
{
	struct MovingResident
	{
		CPU_TRANSFORM m_worldTransform;
		Resident* m_pResident{ nullptr };

		bool isBlocked{ false };
		bool isMoving{ false };
	};

	void RotateEntity(BlitML::fRotation& rotation, float deltaTime, uint32_t movingObjectID);
}