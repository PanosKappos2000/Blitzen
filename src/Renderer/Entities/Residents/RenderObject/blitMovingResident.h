#pragma once

#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	struct MovingResident
	{
		uint32_t m_renderObjectID;

		BlitML::mat4 cpu_worldTransform;

		bool isBlocked{ false };
		bool isMoving{ false };
	};

	void RotateEntity(BlitML::fRotation& rotation, float deltaTime, uint32_t movingObjectID);
}