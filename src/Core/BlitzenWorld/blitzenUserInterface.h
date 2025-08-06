#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Resources/blitShaderShared.h"
#include "Renderer/View/blitCamera.h"
#include "Renderer/Resources/blitResidentData.h"

namespace BlitzenEngine
{
	void LogFrameEventScan(Resident resident);

	void LogFrameEventPfn(Resident resident);

	void LogCollisionEvent(Resident resident);

	void AddCollisionReactionFlag(const char* name);

	// Adds world variable to the list that includes all residents with gravity.
	// This means that the resident will be pulled to the ground constantly, 
	// unless something deactivates its gravity flag (like jump functions)
	void LogResidentForGravity(Resident resident, float maxSpeed);

	// Creates collider for resident. This collider will be used to check it against other residents
	// Residents that have never called this (or future extensions) will be invincible
	void LogResidentForCollision(Resident resident, BlitzenColliderType type);

	void RotateEntity(uint32_t residentID, const BlitML::fRotation& rotation, float deltaTime, uint32_t rotationFlags);

	// Changes resident yaw
	void SetResidentYaw(Resident resident, float yaw);

	// After this is called and until it is deactivated the resident will rotate according to their direction
	// To disable this call StopResidentRotationFromFollowingDirection
	void SetResidentRotationToFollowDirection(Resident resident);

	// Disables the flag that makes resident's orientation follow their direction
	void StopResidentRotationFromFollowingDirection(Resident resident);

	// Takes resident, deltaTime and yaw. Increments that resident's euler angles (basic orientation) on the Y Axis by yaw * deltaTime
	void RotateResidentYaw(Resident resident, float yaw, float deltaTime);

	void KillResidentYawRotation(Resident resident);

	// Takes resident, deltaTime and pitch. Increments that resident's euler angles (basic orientation) on the X Axis by pitch * deltaTime
	void RotateResidentPitch(Resident resident, float pitch, float deltaTime);

	// Takes resident, deltaTime and roll. Increments that resident's euler angles (basic orientation) on the Z Axis by roll * deltaTime 
	void RotateResidentRoll(Resident resident, float pitch, float deltaTime);

	// Sets the logic with which the resident gets its direction when moving
	// Camera: Gets the camera's yaw angle and creates a forward vector
	void SetResidentDirectionInfluence(Resident resident, DirectionInfluencer directionInfluencer);

	// Every time this is called the speed of the resident on the z axis is incremented by their acceleration.
	// Once the max speed stat is reached, the resident will move at a static speed
	void AddResidentVelocityZAxis(Resident resident, float deltaTime);

	// Every time this is called, the speed of the resident on the z axis is incremented by their acceleration negated.
	// Once the max speed stat is reached, the resident will move at a static speed
	void AddResidentVelocityZAxisNegative(Resident resident, float deltaTime);

	// Sets velocity to 0 for ZAxis movement
	void KillResidentVelocityZAxis(Resident resident);

	// Every time this is called the speed of the resident on the x axis is incremented by their acceleration.
	// Once the max speed stat is reached, the resident will move at a static speed.
	void AddResidentVelocityXAxis(Resident resident, float deltaTime);

	// Every time this is called the speed of the resident on the x axis is incremented by their acceleration.
	// Once the max speed stat is reached, the resident will move at a static speed.
	void AddResidentVelocityXAxisNegative(Resident resident, float deltaTime);

	// Sets velocity to 0 for XAxis movement
	void KillResidentVelocityXAxis(Resident resident);

	// This sets a world variable's acceleration stat. This stat is used for interpolation functions
	void SetResidentAcceleration(Resident resident, float acceleration);

	// Returns a world variable's acceleration stat
	float GetResidentAcceleration(Resident resident);

	// Sets a world variable's max speed stat. This stat is true for movement on every axis (except for y axis gravity, which is different)
	void SetResidentMaxVelocity(Resident resident, float maxVelocity);

	// Returns a world variable's max speed stat
	float GetResidentMaxVelocity(Resident resident);

	bool CheckResidentIsFalling(Resident resident);

	bool CheckResidentVelocity(Resident resident);

	void ResidentJump(Resident resident);

	BlitML::fVelocity GetResidentVelocity(Resident resident);

	BlitML::float3 GetResidentPosition(Resident resident);

	BlitML::fRotation GetResidentRotation(Resident resident);

	void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, const BlitML::vec3& initialCameraPosition,
		float drawDistance, float initialYawRotation, float initialPitchRotation);

	BLIT_OFFLINE_FUNC void RegisterFrameEventForWorldVariableType(BlitzenEngine::WorldVariableType resident, BlitzenEngine::ResidentFrameEventPfn function);
}

namespace BlitzenWorld
{
	// Rotates the camera's yaw and pitch, based on movementX and movementY (x goes to pitch, y goes to yaw)
	// Depending on the settings of the resident that it is attached to, it may rotate that resident's yaw as well
	void RequestGameCameraRotation(BlitzenEngine::Resident resident, int32_t movementX, int32_t movementY);

	// Sets up the settings for a camera attachment (camera attached to resident / main character)
	// Padding from attachment is the distance the resident will have from the attached camera
	// CAMERA_FREE_ROTATION_SETTING, dictates when the camera can change the resident's orientation
	void SetupCameraAttachment(uint32_t residentID, BlitML::float3 paddingFromAttachment, bool residentDirectionEffectFlag);

	// Spectator camera movement. Not attached to resident
	void MoveCameraReleased(BlitML::float3 movement);

	BlitML::fDirection GetResidentForward(BlitzenEngine::Resident resident);
}