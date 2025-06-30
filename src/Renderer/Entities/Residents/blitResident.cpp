#pragma once
#include "blitResidentManager.h"
#include "Core/DbLog/blitAssert.h"
#include "Renderer/Resources/Mesh/blitMeshes.h"
#include "Renderer/Entities/Interface/blitComponents.h"
#include "BlitCL/blitDynamicArr.h"

namespace BlitzenEngine
{
	inline static WORLD_RESIDENTS* P_WORLD_RESIDENTS{ nullptr };

	RESIDENT_CREATE_RES WORLD_RESIDENTS::AddResident(const RESIDENT_CREATE_CONTEXT& ctx)
	{
		RenderObject* pFirstRender{ nullptr };

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
		auto boundingSpheresArr{ GetBoundingSphereResources_STATIC_ACCESS(ctx.m_pResource) };

		BlitCL::DynamicArray<RENDER_OBJECT_CREATE_CONTEXT> renderContext{ ctx.m_pResource->surfaceCount };

		// Resolve render object types for resident
		if (ctx.m_isMoveable)
		{
			for (uint32_t prim = 0; prim < ctx.m_pResource->surfaceCount; ++prim)
			{
				renderContext[prim].m_type = RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC;
			}
		}
		else
		{
			for (uint32_t prim = 0; prim < ctx.m_pResource->surfaceCount; ++prim)
			{
				renderContext[prim].m_type = GetMeshPrimitiveTransparencyFlag_STATIC_ACCESS(ctx.m_pResource->firstSurface + prim) == BlitzenCore::FAT_FALSE ?
					RENDER_OBJECT_TYPE::OPAQUE_STATIC : RENDER_OBJECT_TYPE::TRANSPARENT_STATIC;
			}
		}

		for (uint32_t prim = 0; prim < ctx.m_pResource->surfaceCount; ++prim)
		{
			renderContext[prim].m_primitiveID = prim + ctx.m_pResource->firstSurface;
			renderContext[prim].m_transformID = transformID;

			// Creates render object
			uint32_t renderObjectId = m_renders.CreateRenderObject(renderContext[prim]);
			if (renderObjectId == BLIT_MAX_WORLD_RENDERS)
			{
				return RESIDENT_CREATE_RES::RENDER_OBJECT_CREATION_FAILED;
			}

			m_colliders.AddRenderObjectBoundingSphere(&boundingSpheresArr[prim], m_transforms.m_transforms[transformID], renderObjectId, renderContext[prim].m_type != RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC);

			// Saves the first render for the resident
			if (prim == 0)
			{
				pFirstRender = &m_renders.m_renders[renderObjectId];
			}
		}

		if (pFirstRender == nullptr)
		{
			return RESIDENT_CREATE_RES::UNKNOWN;
		}

		m_residents[m_residentCount++] = transformID;

		return RESIDENT_CREATE_RES::SUCCESS;
	}

	RESIDENT_CREATE_RES WORLD_RESIDENTS::AddWorldVariable(const WORLD_VARIABLE_CREATE_CONTEXT& ctx)
	{
		auto baseResidentResidentRes = AddResident(ctx.residentCtx);
		if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)baseResidentResidentRes))
		{
			return baseResidentResidentRes;
		}

		if (m_worldVariableCount >= BlitzenCore::Ce_MaxWorldVariableCount)
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
		BLIT_ASSERT(resident < BlitzenCore::Ce_MaxWorldMovingResidentCount + CE_DYNAMIC_TRANSFORM_OFFSET && resident >= CE_DYNAMIC_TRANSFORM_OFFSET);

		return &P_WORLD_RESIDENTS->m_movingResidents[resident];
	}

	void InitializeWorldResidentsPointer_STATIC_ACCESS(WORLD_RESIDENTS* ptr)
	{
		BLIT_ASSERT(P_WORLD_RESIDENTS == nullptr);

		P_WORLD_RESIDENTS = ptr;
	}

	CPU_TRANSFORM& GetWorldTransform_STATIC_ACCESS(Resident resident)
	{
		BLIT_ASSERT(resident < BlitzenCore::Ce_MaxWorldMovingResidentCount + CE_DYNAMIC_TRANSFORM_OFFSET && resident >= CE_DYNAMIC_TRANSFORM_OFFSET);

		return P_WORLD_RESIDENTS->m_transforms.m_moveables[resident];
	}

	RESIDENT_CREATE_RES AddResident_STATIC_ACCESS(const RESIDENT_CREATE_CONTEXT& ctx)
	{
		return P_WORLD_RESIDENTS->AddResident(ctx);
	}

	void RotateEntity(uint32_t residentID, const BlitML::fRotation& rotation, float deltaTime, uint32_t rotationFlags)
	{
		auto& rotating{ P_WORLD_RESIDENTS->m_transforms.m_moveables[residentID] };

		rotating.rotatingFlags = rotationFlags;
		if (!P_WORLD_RESIDENTS->m_movingResidents[residentID].m_isBlocked)
		{
			P_WORLD_RESIDENTS->m_transforms.m_moveables[residentID].eulerAngles += rotation * deltaTime;
			//AddMovingResident_STATIC_ACCESS(&moving);
		}
	}
}