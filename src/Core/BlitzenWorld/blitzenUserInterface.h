#pragma once

#include "Renderer/Entities/Residents/blitResident.h"

namespace BlitzenEngine
{
	void LogFrameEventScan(Resident resident);

	void LogFrameEventPfn(Resident resident);

	void LogCollisionEvent(Resident resident);

	void AddCollisionReactionFlag(const char* name);

	void RotateEntity(uint32_t residentID, const BlitML::fRotation& rotation, float deltaTime, uint32_t rotationFlags);

	void AddResidentVelocity(Resident resident, const BlitML::fVelocity& velocity, float deltaTime);

	void ResidentCutVelocityForward(Resident resident);

	void ResidentCutVelocityRight(Resident resident);

	void ResidentCutVelocityLeft(Resident resident);

	void ResidentCutVelocityBack(Resident resident);

	bool CheckResidentVelocity(Resident resident);

	BlitML::fVelocity GetResidentVelocity(Resident resident);

	BlitML::float3 GetResidentPosition(Resident resident);

	BlitML::fRotation GetResidentRotation(Resident resident);

	void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, const BlitML::vec3& initialCameraPosition,
		float drawDistance, float initialYawRotation, float initialPitchRotation);
}

namespace BlitzenWorld
{
	void RotateResidentAttachedCamera(BlitzenEngine::Resident resident, int32_t movementX, int32_t movementY);

	void SetupCameraAttachment(uint32_t residentID, BlitML::float3 paddingFromAttachment, BlitzenEngine::CAMERA_FREE_ROTATION_SETTING freeRotationWhen);

	void MoveCameraReleased(BlitML::float3 movement);
}