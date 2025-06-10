#pragma once

#include "Core/BlitzenWorld/blitzenWorldPrivate.h"

namespace BlitzenEngine
{
	void RotateEntity(DynamicTransform* pTransform, BlitML::fRotation& rotation, BlitML::float3& velocity, float deltaTime, uint32_t worldVariableID)
	{
        if (pTransform->m_isMoving && pTransform->m_rotation == rotation && pTransform->m_velocity == velocity)
        {
            BLIT_WARN("Call for rotate on already moving object, transform id: %u, dynamic id: %u", pTransform->m_transformID, pTransform->m_thisID);
            return;
        }

        pTransform->m_rotation = rotation;
        pTransform->m_velocity = velocity;

        BlitzenWorld::S_WORLD_UPDATE_RECEIVER_SEND_TRANSFORM(pTransform, worldVariableID);
	}
}