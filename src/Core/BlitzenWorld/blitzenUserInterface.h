#pragma once

#include "Renderer/Entities/Residents/blitResident.h"

namespace BlitzenEngine
{
	void LogFrameEventScan(Resident resident);

	void LogFrameEventPfn(Resident resident);

	void LogCollisionEvent(Resident resident);

	void AddCollisionReactionFlag(const char* name);

	void LogResidentForGravity(Resident resident, float maxSpeed);

	void RotateEntity(uint32_t residentID, const BlitML::fRotation& rotation, float deltaTime, uint32_t rotationFlags);

	// Takes resident, deltaTime and yaw. Increments that resident's euler angles (basic orientation) on the Y Axis by yaw * deltaTime
	void RotateResidentYaw(Resident resident, float yaw, float deltaTime);

	// Takes resident, deltaTime and pitch. Increments that resident's euler angles (basic orientation) on the X Axis by pitch * deltaTime
	void RotateResidentPitch(Resident resident, float pitch, float deltaTime);

	// Takes resident, deltaTime and roll. Increments that resident's euler angles (basic orientation) on the Z Axis by roll * deltaTime 
	void RotateResidentRoll(Resident resident, float pitch, float deltaTime);

	void AddResidentVelocity(Resident resident, const BlitML::fVelocity& velocity, float deltaTime);

	bool CheckResidentIsFalling(Resident resident);

	bool CheckResidentVelocity(Resident resident);

	void ResidentJump(Resident resident);

	BlitML::fVelocity GetResidentVelocity(Resident resident);

	BlitML::float3 GetResidentPosition(Resident resident);

	BlitML::fRotation GetResidentRotation(Resident resident);

	void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, const BlitML::vec3& initialCameraPosition,
		float drawDistance, float initialYawRotation, float initialPitchRotation);
}

namespace BlitzenWorld
{
	// Rotates the camera's yaw and pitch, based on movementX and movementY (x goes to pitch, y goes to yaw)
	// Depending on the settings of the resident that it is attached to, it may rotate that resident's yaw as well
	void RotateResidentAttachedCamera(BlitzenEngine::Resident resident, int32_t movementX, int32_t movementY);

	// Sets up the settings for a camera attachment (camera attached to resident / main character)
	// Padding from attachment is the distance the resident will have from the attached camera
	// CAMERA_FREE_ROTATION_SETTING, dictates when the camera can change the resident's orientation
	void SetupCameraAttachment(uint32_t residentID, BlitML::float3 paddingFromAttachment, BlitzenEngine::CAMERA_FREE_ROTATION_SETTING freeRotationWhen);

	// Spectator camera movement. Not attached to resident
	void MoveCameraReleased(BlitML::float3 movement);
}