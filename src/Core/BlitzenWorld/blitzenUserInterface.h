#pragma once

#include "Renderer/Entities/Residents/blitResident.h"

namespace BlitzenEngine
{
	void LogFrameEventScan(Resident resident);

	void LogFrameEventPfn(Resident resident);

	void LogCollisionEvent(Resident resident);

	void AddCollisionReactionFlag(const char* name);

	void RotateEntity(uint32_t residentID, const BlitML::fRotation& rotation, float deltaTime, uint32_t rotationFlags);

	void MoveResident(Resident resident, const BlitML::fVelocity& velocity, float deltaTime);

	void AddResidentVelocity(Resident resident, const BlitML::fVelocity& velocity, float deltaTime);
}