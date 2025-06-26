#pragma once
#include "Renderer/Entities/Residents/blitResident.h"
#include "Renderer/Entities/Residents/Dynamic/blitMovingResident.h"

namespace BlitzenEngine
{
	class WVRotatingKitten
	{
	public:
		void Start();

		void Tick(float deltaTime);
	private:
		uint32_t residentID;

		MovingResident* m_pMovingResident;

		float speed = 1.f;
	};
}
