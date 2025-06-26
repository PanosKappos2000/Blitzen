#pragma once
#include "blitResidentManager.h"
#include "Core/DbLog/blitAssert.h"
#include "Renderer/Resources/Mesh/blitMeshes.h"

namespace BlitzenEngine
{
	inline WORLD_RESIDENTS* pWorldResidents_STATIC_ACCESS{ nullptr };

	RESIDENT_CREATE_RES WORLD_RESIDENTS::AddResident(const RESIDENT_CREATE_CONTEXT& ctx)
	{
		RenderObject* pFirstRender{ nullptr };

		uint32_t transformID{ m_transforms.CreateTransform(ctx.m_transformInfo) };
		if (transformID == BLIT_MAX_WORLD_TRANSFORM_COUNT)
		{
			return WORLD_TRANSFORM_CREATION_FAILED;
		}

		// Retrieves bounding spheres array
		auto boundingSpheresArr{ GetBoundingSphereResources_STATIC_ACCESS(ctx.m_pResource) };

		for (uint32_t prim = 0; prim < ctx.m_pResource->surfaceCount; ++prim)
		{
			RENDER_OBJECT_CREATE_CONTEXT renderContext{};
			if (ctx.m_isMoveable)
			{
				renderContext.m_type = RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC;
			}
			else
			{
				renderContext.m_type = GetMeshPrimitiveTransparencyFlag_STATIC_ACCESS(ctx.m_pResource->firstSurface + prim) == BlitzenCore::FAT_FALSE ? 
					RENDER_OBJECT_TYPE::OPAQUE_STATIC : RENDER_OBJECT_TYPE::TRANSPARENT_STATIC;
			}
			renderContext.m_primitiveID = prim + ctx.m_pResource->firstSurface;
			renderContext.m_transformID = transformID;

			// Creates render object
			uint32_t renderObjectId = m_renders.CreateRenderObject(renderContext);
			if (renderObjectId == BLIT_MAX_WORLD_RENDERS)
			{
				return RENDER_OBJECT_CREATION_FAILED;
			}

			m_colliders.AddRenderObjectBoundingSphere(&boundingSpheresArr[prim], m_transforms.m_transforms[transformID], renderObjectId, renderContext.m_type != RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC);

			// Saves the first render for the resident
			if (prim == 0)
			{
				pFirstRender = &m_renders.m_renders[renderObjectId];
			}
		}

		if (pFirstRender == nullptr)
		{
			return UNKNOWN;
		}

		m_residents[m_residentCount] = CreateResident(pFirstRender, ctx.m_pResource->surfaceCount);

		return SUCCESS;
	}

	Resident CreateResident(RenderObject* pRender, uint32_t renderCount)
	{
		Resident res{};
		res.m_pRender = pRender;
		res.m_count = renderCount;

		return res;
	}

	MovingResident* RequestMovementComponent()
	{
		auto& transforms{ pWorldResidents_STATIC_ACCESS->m_transforms };
		BLIT_ASSERT(transforms.m_moveableCount < BlitzenCore::Ce_MaxWorldMovingResidentCount);

		CPU_TRANSFORM* pTransform = pWorldResidents_STATIC_ACCESS->m_transforms.SwitchLastToDynamic();

		return &pWorldResidents_STATIC_ACCESS->m_movingResidents[transforms.m_moveableCount];
	}

	void InitializeWorldResidentsPointer_STATIC_ACCESS(WORLD_RESIDENTS* ptr)
	{
		BLIT_ASSERT(pWorldResidents_STATIC_ACCESS == nullptr);

		pWorldResidents_STATIC_ACCESS = ptr;
	}

	CPU_TRANSFORM& GetWorldTransform_STATIC_ACCESS(uint32_t residentID)
	{
		return pWorldResidents_STATIC_ACCESS->m_transforms.m_moveables[pWorldResidents_STATIC_ACCESS->m_residents[residentID].m_pRender->transformId];
	}

	RESIDENT_CREATE_RES AddResident_STATIC_ACCESS(const RESIDENT_CREATE_CONTEXT& ctx)
	{
		return pWorldResidents_STATIC_ACCESS->AddResident(ctx);
	}
}