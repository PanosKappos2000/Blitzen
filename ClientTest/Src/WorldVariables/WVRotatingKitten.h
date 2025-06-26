#pragma once
#include "Renderer/Entities/Residents/blitResident.h"
#include "Renderer/Entities/Residents/Dynamic/blitMovingResident.h"

namespace BlitzenEngine
{
	class WVRotatingKitten
	{
	public:
		uint32_t RESIDENT_ID;
	public:
		void Start();

		void Tick(float deltaTime);
	private:

		MovingResident* m_pMovingResident;

		float speed = 1.f;
	};
}
