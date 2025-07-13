#pragma once
#include "blitResidentManager.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/DbLog/blitLogger.h"
#include "Renderer/Resources/Mesh/blitMeshes.h"
#include "Renderer/Entities/Interface/blitComponents.h"
#include "BlitCL/blitDynamicArr.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"
#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenEngine
{
	inline static WORLD_RESIDENTS* GSWorldResidents{ nullptr };

	RESIDENT_CREATE_RES WORLD_RESIDENTS::AddResident(const RESIDENT_CREATE_CONTEXT& ctx)
	{
		RenderObject* pFirstRender{ nullptr };

		bool transparencyFlag = GetMeshPrimitiveTransparencyFlag_STATIC_ACCESS(ctx.m_resourceID) == BlitzenCore::FAT_TRUE;
		if (transparencyFlag)
		{
			const_cast<TRANSFORM_CREATE_CONTEXT&>(ctx.m_transformInfo).m_type = WorldTransformType::BOUND_TO_TRANSPARENT;
		}

		uint32_t transformID{ m_transforms.CreateTransform(ctx.m_transformInfo) };
		if (transformID == BLIT_MAX_WORLD_TRANSFORM_COUNT)
		{
			return RESIDENT_CREATE_RES::WORLD_TRANSFORM_CREATION_FAILED;
		}
		if (ctx.m_transformInfo.m_type == WorldTransformType::DYNAMIC)
		{
			
		}

		// Retrieves bounding spheres array
		auto bounds{ GetBoundingSphere_STATIC_ACCESS(ctx.m_resourceID) };

		RENDER_OBJECT_CREATE_CONTEXT renderContext{};
		renderContext.m_type = ctx.m_isMoveable ? RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC : RENDER_OBJECT_TYPE::OPAQUE_STATIC;
		if (renderContext.m_type == RENDER_OBJECT_TYPE::OPAQUE_STATIC)
		{
			renderContext.m_type = !transparencyFlag ? RENDER_OBJECT_TYPE::OPAQUE_STATIC : RENDER_OBJECT_TYPE::TRANSPARENT_STATIC;
		}

		renderContext.m_primitiveID = ctx.m_resourceID;
		renderContext.m_transformID = transformID;

		// Creates render object
		uint32_t renderObjectId = m_renders.CreateRenderObject(renderContext);
		if (renderObjectId == BLIT_MAX_WORLD_RENDERS)
		{
			return RESIDENT_CREATE_RES::RENDER_OBJECT_CREATION_FAILED;
		}

		m_colliders.AddRenderObjectBoundingSphere(&bounds, m_transforms.m_transforms[transformID], renderObjectId, renderContext.m_type);

		m_residents[m_residentCount++] = renderObjectId;

		return RESIDENT_CREATE_RES::SUCCESS;
	}

	RESIDENT_CREATE_RES WORLD_RESIDENTS::AddWorldVariable(const WORLD_VARIABLE_CREATE_CONTEXT& ctx)
	{
		auto baseResidentResidentRes = AddResident(ctx.residentCtx);
		if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)baseResidentResidentRes))
		{
			return baseResidentResidentRes;
		}

		if (MWorldVariableCount >= BLIT_MAX_WORLD_VARIABLE_COUNT)
		{
			return RESIDENT_CREATE_RES::WORLD_VARIABLE_COUNT_EXCEEDED;
		}

		MWorldVariables[MWorldVariableCount] = ctx.m_worldVariableID;
		MWorldVariableCount++;

		return RESIDENT_CREATE_RES::SUCCESS;
	}

	void WORLD_RESIDENTS::UpdateMovingResidents(float deltaTime)
	{
		
	}

	void WORLD_RESIDENTS::UpdateFallingResidents(float deltaTime)
	{
		for (uint32_t wv = 0; wv < WVWithGravityCount; wv++)
		{
			uint32_t IDX = WVWithGravityIDXs[wv];
			auto& gravityData = WVGravityData[IDX];
			if (m_transforms.WVWithMovement[IDX].movementFlags & BLIT_RESIDENT_MOVEMENT_FALLING_BIT)
			{
				m_transforms.WVWithMovement[IDX].position.y -= gravityData.currentSpeed * deltaTime;

				// TEMP
				gravityData.currentSpeed = BlitML::FMax(gravityData.maxSpeed, gravityData.currentSpeed + BLIT_GRAVITATIONAL_ACCELERATION);
			}
			else
			{
				gravityData.currentSpeed = 0.f;
			}
		}
	}

	MovingResident* RequestMovementComponent(Resident resident)
	{
		BLIT_ASSERT(resident < BLIT_MAX_WORLD_VARIABLE_COUNT + CE_DYNAMIC_TRANSFORM_OFFSET && resident >= CE_DYNAMIC_TRANSFORM_OFFSET);

		return &GSWorldResidents->m_movingResidents[resident];
	}

	void InitializeWorldResidentsPointer_STATIC_ACCESS(WORLD_RESIDENTS* ptr)
	{
		BLIT_ASSERT(GSWorldResidents == nullptr);

		GSWorldResidents = ptr;
	}

	RESIDENT_CREATE_RES AddResident_STATIC_ACCESS(const RESIDENT_CREATE_CONTEXT& ctx)
	{
		return GSWorldResidents->AddResident(ctx);
	}

	void RotateEntity(uint32_t residentID, const BlitML::fRotation& rotation, float deltaTime, uint32_t rotationFlags)
	{
		auto& rotating{ GSWorldResidents->m_transforms.WVWithMovement[residentID] };

		rotating.movementFlags |= rotationFlags;
		GSWorldResidents->m_transforms.WVWithMovement[residentID].eulerAngles += rotation * deltaTime;
	}

	void RotateResidentYaw(Resident resident, float yaw, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		GSWorldResidents->m_transforms.WVWithMovement[resident].eulerAngles.y += yaw * deltaTime;
		GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags |= BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT;
	}

	void KillResidentYawRotation(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);
		
		GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags &= ~(BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT);
	}

	void RotateResidentPitch(Resident resident, float pitch, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		GSWorldResidents->m_transforms.WVWithMovement[resident].eulerAngles.x += pitch * deltaTime;
		GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags |= BLIT_RESIDENT_MOVEMENT_ROTATING_PITCH_BIT;
	}

	void RotateResidentRoll(Resident resident, float roll, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		GSWorldResidents->m_transforms.WVWithMovement[resident].eulerAngles.z += roll * deltaTime;
		GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags |= BLIT_RESIDENT_MOVEMENT_ROTATING_ROLL_BIT;
	}

	void AddResidentVelocity(Resident resident, const BlitML::fVelocity& velocity, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		auto& residentData = GSWorldResidents->m_transforms.WVWithMovement[resident];

		BlitML::float3 worldMove = BlitML::ToVec3(BlitML::Mat4EulerY(residentData.eulerAngles.y) * BlitML::vec4{ velocity, 0.0f });

		GSWorldResidents->m_transforms.WVWithMovement[resident].position += worldMove * 0.01f;
	}

	void AddResidentVelocityZAxis(Resident resident, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		auto& accelerationData = GSWorldResidents->WVVelocityData[resident];
		auto& transform = GSWorldResidents->m_transforms.WVWithMovement[resident];

		float velocity = BlitML::FMax(accelerationData.currentSpeed + accelerationData.acceleration, accelerationData.maxSpeed);
		velocity *= deltaTime;
		transform.position += BlitML::ToVec3(BlitML::Mat4EulerY(transform.eulerAngles.y) * BlitML::vec4{ 0.f, 0.f, velocity, 0.0f });
		transform.movementFlags |= BLIT_RESIDENT_MOVEMENT_VELOCITY_ZAXIS_BIT;
	}

	void AddResidentVelocityZAxisNegative(Resident resident, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		auto& accelerationData = GSWorldResidents->WVVelocityData[resident];
		auto& transform = GSWorldResidents->m_transforms.WVWithMovement[resident];

		float velocity = BlitML::FMin(accelerationData.currentSpeed - accelerationData.acceleration, -accelerationData.maxSpeed);
		velocity *= deltaTime;
		transform.position += BlitML::ToVec3(BlitML::Mat4EulerY(transform.eulerAngles.y) * BlitML::vec4{ 0.f, 0.f, velocity, 0.0f });
		transform.movementFlags |= BLIT_RESIDENT_MOVEMENT_VELOCITY_ZAXIS_BIT;
	}

	void KillResidentVelocityZAxis(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags &= ~(BLIT_RESIDENT_MOVEMENT_VELOCITY_ZAXIS_BIT);
		if (!(GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags & BLIT_RESIDENT_MOVEMENT_VELOCITY_XAXIS_BIT))
		{
			GSWorldResidents->WVVelocityData[resident].currentSpeed = 0.f;
		}
	}

	void AddResidentVelocityXAxis(Resident resident, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		auto& accelerationData = GSWorldResidents->WVVelocityData[resident];
		auto& transform = GSWorldResidents->m_transforms.WVWithMovement[resident];

		float velocity = BlitML::FMax(accelerationData.currentSpeed + accelerationData.acceleration, accelerationData.maxSpeed);
		velocity *= deltaTime;
		transform.position += BlitML::ToVec3(BlitML::Mat4EulerY(transform.eulerAngles.y) * BlitML::vec4{ velocity, 0.f, 0.f, 0.0f });
		transform.movementFlags |= BLIT_RESIDENT_MOVEMENT_VELOCITY_XAXIS_BIT;
	}

	void AddResidentVelocityXAxisNegative(Resident resident, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		auto& accelerationData = GSWorldResidents->WVVelocityData[resident];
		auto& transform = GSWorldResidents->m_transforms.WVWithMovement[resident];

		float velocity = BlitML::FMin(accelerationData.currentSpeed - accelerationData.acceleration, -accelerationData.maxSpeed);
		velocity *= deltaTime;
		transform.position += BlitML::ToVec3(BlitML::Mat4EulerY(transform.eulerAngles.y) * BlitML::vec4{ velocity, 0.f, 0.f, 0.0f });
		transform.movementFlags |= BLIT_RESIDENT_MOVEMENT_VELOCITY_XAXIS_BIT;
	}

	void KillResidentVelocityXAxis(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags &= ~(BLIT_RESIDENT_MOVEMENT_VELOCITY_XAXIS_BIT);
		if (!(GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags & BLIT_RESIDENT_MOVEMENT_VELOCITY_ZAXIS_BIT))
		{
			GSWorldResidents->WVVelocityData[resident].currentSpeed = 0.f;
		}
	}

	void SetResidentAcceleration(Resident resident, float acceleration)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		GSWorldResidents->WVVelocityData[resident].acceleration = acceleration;
	}

	float GetResidentAcceleration(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->m_transforms.m_moveableCount);

		return GSWorldResidents->WVVelocityData[resident].acceleration;
	}

	void SetResidentMaxVelocity(Resident resident, float maxVelocity)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->m_transforms.m_moveableCount);

		GSWorldResidents->WVVelocityData[resident].maxSpeed = maxVelocity;
	}

	float GetResidentMaxVelocity(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->m_transforms.m_moveableCount);

		return GSWorldResidents->WVVelocityData[resident].maxSpeed;
	}

	BlitML::fVelocity GetResidentVelocity(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->m_transforms.m_moveableCount);

		return GSWorldResidents->m_transforms.WVWithMovement[resident].position;
	}

	BlitML::float3 GetResidentPosition(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->m_transforms.m_moveableCount);

		return GSWorldResidents->m_transforms.WVWithMovement[resident].position;
	}

	bool CheckResidentVelocity(Resident resident)
	{
		if (resident > GSWorldResidents->m_residentCount)
		{
			return false;
		}

		return GSWorldResidents->WVVelocityData[resident].currentSpeed != 0.f;
	}

	bool CheckResidentIsFalling(Resident resident)
	{
		if (resident > GSWorldResidents->MWorldVariableCount)
		{
			return false;
		}

		return GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags & BLIT_RESIDENT_MOVEMENT_FALLING_BIT;
	}

	BlitML::fRotation GetResidentRotation(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->m_transforms.m_moveableCount);

		return GSWorldResidents->m_transforms.WVWithMovement[resident].eulerAngles;
	}

	void LogResidentForGravity(Resident resident, float maxSpeed)
	{
		BLIT_ASSERT(resident < BLIT_MAX_WORLD_VARIABLE_COUNT);

		GSWorldResidents->m_transforms.WVWithMovement[resident].movementFlags |= BLIT_RESIDENT_MOVEMENT_GRAVITY_BIT;
		GSWorldResidents->WVWithGravityIDXs[GSWorldResidents->WVWithGravityCount++] = resident;
		GSWorldResidents->WVGravityData[resident].maxSpeed = maxSpeed;
	}
}