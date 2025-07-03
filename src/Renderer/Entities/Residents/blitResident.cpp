#pragma once
#include "blitResidentManager.h"
#include "Core/DbLog/blitAssert.h"
#include "Renderer/Resources/Mesh/blitMeshes.h"
#include "Renderer/Entities/Interface/blitComponents.h"
#include "BlitCL/blitDynamicArr.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"

namespace BlitzenEngine
{
	inline static WORLD_RESIDENTS* P_WORLD_RESIDENTS{ nullptr };

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
			m_movingResidents[transformID].m_isBlocked = BLIT_FAT_FALSE;
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

		m_colliders.AddRenderObjectBoundingSphere(&bounds, m_transforms.m_transforms[transformID], renderObjectId, renderContext.m_type != RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC);

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

		if (m_worldVariableCount >= BLIT_MAX_WORLD_VARIABLE_COUNT)
		{
			return RESIDENT_CREATE_RES::WORLD_VARIABLE_COUNT_EXCEEDED;
		}

		m_worldVariables[m_worldVariableCount].m_worldVariableID = ctx.m_worldVariableID;
		m_worldVariables[m_worldVariableCount].m_engineResidentID = m_residents[m_residentCount - 1];
		m_worldVariableCount++;

		return RESIDENT_CREATE_RES::SUCCESS;
	}

	MovingResident* RequestMovementComponent(Resident resident)
	{
		BLIT_ASSERT(resident < BLIT_MAX_WORLD_VARIABLE_COUNT + CE_DYNAMIC_TRANSFORM_OFFSET && resident >= CE_DYNAMIC_TRANSFORM_OFFSET);

		return &P_WORLD_RESIDENTS->m_movingResidents[resident];
	}

	void InitializeWorldResidentsPointer_STATIC_ACCESS(WORLD_RESIDENTS* ptr)
	{
		BLIT_ASSERT(P_WORLD_RESIDENTS == nullptr);

		P_WORLD_RESIDENTS = ptr;
	}

	CPU_TRANSFORM& GetWorldTransform_STATIC_ACCESS(Resident resident)
	{
		BLIT_ASSERT(resident < BLIT_MAX_WORLD_VARIABLE_COUNT + CE_DYNAMIC_TRANSFORM_OFFSET && resident >= CE_DYNAMIC_TRANSFORM_OFFSET);

		return P_WORLD_RESIDENTS->m_transforms.m_moveables[resident];
	}

	RESIDENT_CREATE_RES AddResident_STATIC_ACCESS(const RESIDENT_CREATE_CONTEXT& ctx)
	{
		return P_WORLD_RESIDENTS->AddResident(ctx);
	}

	void RotateEntity(uint32_t residentID, const BlitML::fRotation& rotation, float deltaTime, uint32_t rotationFlags)
	{
		auto& rotating{ P_WORLD_RESIDENTS->m_transforms.m_moveables[residentID] };

		rotating.movementFlags |= rotationFlags;
		if (!P_WORLD_RESIDENTS->m_movingResidents[residentID].m_isBlocked)
		{
			P_WORLD_RESIDENTS->m_transforms.m_moveables[residentID].eulerAngles += rotation * deltaTime;
			//AddMovingResident_STATIC_ACCESS(&moving);
		}

		BLIT_ASSERT(rotating.movementFlags & BLIT_RESIDENT_MOVEMENT_GRAVITY_BIT);
	}
}