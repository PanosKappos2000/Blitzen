#pragma once
#include "blitResidentManager.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/DbLog/blitLogger.h"
#include "Renderer/Resources/Mesh/blitMeshes.h"
#include "BlitCL/blitDynamicArr.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"
#include "BlitzenMathLibrary/blitMLSIMD.h"
#include "Renderer/Resources/Terrain/blitTerrain.h"

namespace BlitzenEngine
{
	inline static WORLD_RESIDENTS* GSWorldResidents{ nullptr };

	RESIDENT_CREATE_RES WORLD_RESIDENTS::AddResident(const RESIDENT_CREATE_CONTEXT& ctx)
	{
		RenderObject* pFirstRender{ nullptr };

		// Checks if the resource that is being used is transparent.
		// Resources that are transparent need to be place in the correct offset on the transform array
		bool transparencyFlag = GetMeshPrimitiveTransparencyFlag_STATIC_ACCESS(ctx.m_resourceID) == BlitzenCore::FAT_TRUE;
		if (transparencyFlag)
		{
			const_cast<TRANSFORM_CREATE_CONTEXT&>(ctx.m_transformInfo).m_type = WorldTransformType::BOUND_TO_TRANSPARENT;
		}

		uint32_t transformID = GCTransformCreateErrorCode;
		// Creates transform, saves ID, checks for error code.
		if (ctx.m_transformInfo.m_type != WorldTransformType::DYNAMIC)
		{
			transformID = mTransforms.CreateTransformStatic(ctx.m_transformInfo);
			if (transformID == GCTransformCreateErrorCode)
			{
				return RESIDENT_CREATE_RES::WORLD_TRANSFORM_CREATION_FAILED;
			}
		}
		// World variables cannot bake their transforms
		else if (ctx.m_transformInfo.m_type == WorldTransformType::DYNAMIC)
		{
			transformID = CreateWorldVariableTransform(ctx.pWorldVariableTransform, ctx.m_transformInfo.m_pTransform->scale);
			if (transformID == GCWorldVariableTransformErrorCode)
			{
				return RESIDENT_CREATE_RES::WORLD_TRANSFORM_CREATION_FAILED;
			}
		}

		// Gets the visibility bounding sphere. Will get its transform baked for static objects
		auto bounds{ GetVisibilityBoundingSphereFromMeshPrimitive(ctx.m_resourceID) };

		ResidentSnapDown(mTransforms.m_transforms[transformID], bounds.m_radius);

		// Chooses render object type
		RENDER_OBJECT_CREATE_CONTEXT renderContext{};
		renderContext.m_type = ctx.m_isMoveable ? RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC : RENDER_OBJECT_TYPE::OPAQUE_STATIC;
		if (renderContext.m_type == RENDER_OBJECT_TYPE::OPAQUE_STATIC)
		{
			renderContext.m_type = !transparencyFlag ? RENDER_OBJECT_TYPE::OPAQUE_STATIC : RENDER_OBJECT_TYPE::TRANSPARENT_STATIC;
		}

		renderContext.m_primitiveID = ctx.m_resourceID;
		renderContext.m_transformID = transformID;

		// Creates render object. Used to acces
		uint32_t renderObjectId = m_renders.CreateRenderObject(renderContext);
		if (renderObjectId == GCRenderObjectCreationErrorCode)
		{
			return RESIDENT_CREATE_RES::RENDER_OBJECT_CREATION_FAILED;
		}

		// Add bounding sphere for visibility checks
		// It is passed to render objects to avoid transforming it for static objects
		MColliders.AddRenderObjectBoundingSphere(&bounds, mTransforms.m_transforms[transformID], renderObjectId, renderContext.m_type);

		if (renderObjectId != transformID)
		{
			return RESIDENT_CREATE_RES::UNKNOWN;
		}

		if (!(ctx.m_flags & RESIDENT_CREATE_NO_COLLISION))
		{
			auto& collider = GetColliderFromMeshPrimitive(ctx.m_resourceID);
			if (!MColliders.LogResidentForCollision(transformID, collider, mTransforms.m_transforms[transformID]))
			{
				return RESIDENT_CREATE_RES::COLLIDER_CREATION_FAILED;
			}
		}

		// This is a weird one since all arrays should be parallel.
		// Added check above to avoid unexpected behaviour
		mResidents[mResidentCount++] = renderObjectId;

		return RESIDENT_CREATE_RES::SUCCESS;
	}

	uint32_t WORLD_RESIDENTS::CreateWorldVariableTransform(WVTransform* pWVTransform, float scale)
	{
		if (pWVTransform == nullptr)
		{
			BLIT_ERROR("%s: Dynamic transform requested but no CPU transform data passed", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
			return GCWorldVariableTransformErrorCode;
		}

		BlitzenCore::BlitMemCopy(&WVTransforms[mWorldVariableCount], pWVTransform, sizeof(WVTransform));
		
		// GPU data. The offset in the full world transform array should be zero. (Static assert on blitShaderShared.h)
		auto& gpuTransform = mTransforms.m_transforms[mWorldVariableCount];
		// Sync position.
		gpuTransform.pos = pWVTransform->position;
		// World variable version does not carry scale on its cpu struct
		gpuTransform.scale = scale;
		// Quat for render orientation
		BlitML::quat orientationYaw = BlitML::NormalizedQuatFromAngleAxis(BlitML::float3(0.f, -1.f, 0.f), pWVTransform->eulerAngles.x);
		BlitML::quat orientationPitch = BlitML::NormalizedQuatFromAngleAxis(BlitML::float3(1.f, 0.f, 0.f), pWVTransform->eulerAngles.y);
		gpuTransform.orientation = BlitML::MulitplyQuat(orientationYaw, orientationPitch);

		return mWorldVariableCount;
	}

	RESIDENT_CREATE_RES WORLD_RESIDENTS::AddWorldVariable(const WORLD_VARIABLE_CREATE_CONTEXT& ctx)
	{
		if (mWorldVariableCount >= BLIT_MAX_WORLD_VARIABLE_COUNT)
		{
			return RESIDENT_CREATE_RES::WORLD_VARIABLE_COUNT_EXCEEDED;
		}

		auto baseResidentResidentRes = AddResident(ctx.residentCtx);
		if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)baseResidentResidentRes))
		{
			return baseResidentResidentRes;
		}

		Resident resident = mWorldVariableCount;

		MWorldVariables[mWorldVariableCount] = ctx.m_worldVariableID;
		mWorldVariableCount++;

		SetResidentDirectionInfluence(resident, ctx.directionInfluencer);

		WVTransforms[resident].movementFlags |= ctx.residentMovementFlags;

		return RESIDENT_CREATE_RES::SUCCESS;
	}

	void WORLD_RESIDENTS::UpdateMovingResidents(float deltaTime)
	{
		for (uint32_t id = 0; id < mWithVelocityCount; ++id)
		{
			Resident resident = WVWithVelocity[id];
			auto& transform = WVTransforms[resident];
			auto& intent = WVDirectionData[resident];

			if (intent.intent.x == 0.f && intent.intent.z == 0.f)
			{
				// Could improve this for deceleration
				BlitML::DisableStaticArrayElement(WVWithVelocity, resident, mWithVelocityCount);
				WVVelocityData[resident].currentSpeed = 0.f;
				transform.movementFlags &= ~(BLIT_RESIDENT_MOVEMENT_VELOCITY_BIT);
			}
			else
			{
				BlitML::fDirection directionalMovement = BlitzenWorld::GetResidentForward(resident);

				auto& accelerationData = WVVelocityData[resident];
				accelerationData.currentSpeed += accelerationData.acceleration;
				float velocity = BlitML::FMin(accelerationData.currentSpeed, accelerationData.maxSpeed);
				velocity *= deltaTime;

				transform.position += directionalMovement * velocity;
				transform.movementFlags |= BLIT_RESIDENT_MOVEMENT_VELOCITY_BIT;
			}
		}
	}

	void WORLD_RESIDENTS::UpdateFallingResidents(float deltaTime)
	{
		for (uint32_t wv = 0; wv < mWithGravityCount; wv++)
		{
			uint32_t IDX = WVWithGravityIDXs[wv];
			auto& gravityData = WVGravityData[IDX];
			if (WVTransforms[IDX].movementFlags & BLIT_RESIDENT_MOVEMENT_FALLING_BIT)
			{
				WVTransforms[IDX].position.y -= gravityData.currentSpeed * deltaTime;

				// TEMP
				gravityData.currentSpeed = BlitML::FMax(gravityData.maxSpeed, gravityData.currentSpeed + BLIT_GRAVITATIONAL_ACCELERATION);
			}
			else
			{
				gravityData.currentSpeed = 0.f;
			}
		}
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

	void SetResidentYaw(Resident resident, float yaw)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVTransforms[resident].eulerAngles = yaw;
	}

	void SetResidentRotationToFollowDirection(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVTransforms[resident].movementFlags |= BLIT_RESIDENT_MOVEMENT_ROTATE_TO_DIRECTION_BIT;
	}

	void StopResidentRotationFromFollowingDirection(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVTransforms[resident].movementFlags &= ~(BLIT_RESIDENT_MOVEMENT_ROTATE_TO_DIRECTION_BIT);
	}

	void RotateResidentYaw(Resident resident, float yaw, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVTransforms[resident].eulerAngles.y += yaw * deltaTime;
		GSWorldResidents->WVTransforms[resident].movementFlags |= BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT;
	}

	void KillResidentYawRotation(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);
		
		GSWorldResidents->WVTransforms[resident].movementFlags &= ~(BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT);
	}

	void RotateResidentPitch(Resident resident, float pitch, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVTransforms[resident].eulerAngles.x += pitch * deltaTime;
		GSWorldResidents->WVTransforms[resident].movementFlags |= BLIT_RESIDENT_MOVEMENT_ROTATING_PITCH_BIT;
	}

	void RotateResidentRoll(Resident resident, float roll, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVTransforms[resident].eulerAngles.z += roll * deltaTime;
		GSWorldResidents->WVTransforms[resident].movementFlags |= BLIT_RESIDENT_MOVEMENT_ROTATING_ROLL_BIT;
	}

	void SetResidentDirectionInfluence(Resident resident, DirectionInfluencer directionInfluencer)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVDirectionData[resident].directionInfluencer = directionInfluencer;
	}

	void AddResidentVelocityZAxis(Resident resident, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVDirectionData[resident].intent.z = 1.f;

		if (!CheckResidentVelocity(resident))
		{
			GSWorldResidents->WVWithVelocity[GSWorldResidents->mWithVelocityCount++] = resident;
		}
	}

	void AddResidentVelocityZAxisNegative(Resident resident, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVDirectionData[resident].intent.z = -1.f;

		if (!CheckResidentVelocity(resident))
		{
			GSWorldResidents->WVWithVelocity[GSWorldResidents->mWithVelocityCount++] = resident;
		}
	}

	void KillResidentVelocityZAxis(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVDirectionData[resident].intent.z = 0.f;
	}

	void AddResidentVelocityXAxis(Resident resident, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVDirectionData[resident].intent.x = 1.f;

		if (!CheckResidentVelocity(resident))
		{
			GSWorldResidents->WVWithVelocity[GSWorldResidents->mWithVelocityCount++] = resident;
		}
	}

	void AddResidentVelocityXAxisNegative(Resident resident, float deltaTime)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVDirectionData[resident].intent.x = -1.f;

		if (!CheckResidentVelocity(resident))
		{
			GSWorldResidents->WVWithVelocity[GSWorldResidents->mWithVelocityCount++] = resident;
		}
	}

	void KillResidentVelocityXAxis(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVDirectionData[resident].intent.x = 0.f;
	}

	void SetResidentAcceleration(Resident resident, float acceleration)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVVelocityData[resident].acceleration = acceleration;
	}

	float GetResidentAcceleration(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->mWorldVariableCount);

		return GSWorldResidents->WVVelocityData[resident].acceleration;
	}

	void SetResidentMaxVelocity(Resident resident, float maxVelocity)
	{
		BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSWorldResidents->mWorldVariableCount);

		GSWorldResidents->WVVelocityData[resident].maxSpeed = maxVelocity;
	}

	float GetResidentMaxVelocity(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->mWorldVariableCount);

		return GSWorldResidents->WVVelocityData[resident].maxSpeed;
	}

	BlitML::fVelocity GetResidentVelocity(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->mWorldVariableCount);

		return GSWorldResidents->WVTransforms[resident].position;
	}

	BlitML::float3 GetResidentPosition(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->mWorldVariableCount);

		return GSWorldResidents->WVTransforms[resident].position;
	}

	bool CheckResidentVelocity(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->mWorldVariableCount);

		return GSWorldResidents->WVVelocityData[resident].currentSpeed != 0.f;
	}

	bool CheckResidentIsFalling(Resident resident)
	{
		if (resident > GSWorldResidents->mWorldVariableCount)
		{
			return false;
		}

		return GSWorldResidents->WVTransforms[resident].movementFlags & BLIT_RESIDENT_MOVEMENT_FALLING_BIT;
	}

	BlitML::fRotation GetResidentRotation(Resident resident)
	{
		BLIT_RUNTIME_TEST_CHECK_ASSERT(resident < GSWorldResidents->mWorldVariableCount);

		return GSWorldResidents->WVTransforms[resident].eulerAngles;
	}

	void LogResidentForGravity(Resident resident, float maxSpeed)
	{
		BLIT_ASSERT(resident < BLIT_MAX_WORLD_VARIABLE_COUNT);

		GSWorldResidents->WVTransforms[resident].movementFlags |= BLIT_RESIDENT_MOVEMENT_GRAVITY_BIT;
		GSWorldResidents->WVWithGravityIDXs[GSWorldResidents->mWithGravityCount++] = resident;
		GSWorldResidents->WVGravityData[resident].maxSpeed = maxSpeed;
	}
}