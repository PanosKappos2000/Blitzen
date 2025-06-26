#include "WVRotatingKitten.h"

namespace BlitzenEngine
{
	void WVRotatingKitten::Start()
	{
		m_pMovingResident = RequestMovementComponent();
	}

	void WVRotatingKitten::Tick(float deltaTime)
	{
		m_pMovingResident->Rotate(BlitML::fRotation{ deltaTime * speed, 0.f, 0.f }, deltaTime);
	}
}