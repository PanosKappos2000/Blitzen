#pragma once
#include "Renderer/Entities/Residents/blitResident.h"

namespace BlitzenEngine
{
	class MovingResident
	{
	public:

		CPU_TRANSFORM* m_pWorldTransform;

		bool isBlocked{ false };
		bool isMoving{ false };

		void Rotate(BlitML::fRotation& rotation, float deltaTime);
	};

	MovingResident& GetMovingResident_STATIC_ACCESS(uint32_t residentID);
}