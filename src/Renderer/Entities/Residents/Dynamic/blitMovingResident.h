#pragma once
#include "Renderer/Entities/Residents/blitResident.h"

namespace BlitzenEngine
{
	class MovingResident
	{
	public:
		
	};

	MovingResident& GetMovingResident_STATIC_ACCESS(uint32_t residentID);

	MovingResident* RequestMovementComponent(uint32_t residentID);
}