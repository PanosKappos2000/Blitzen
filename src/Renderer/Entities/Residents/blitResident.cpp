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
			renderContext.m_type = ctx.m_renderTypes[prim];
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

	void InitializeWorldResidentsPointer_STATIC_ACCESS(WORLD_RESIDENTS* ptr)
	{
		BLIT_ASSERT(pWorldResidents_STATIC_ACCESS == nullptr);

		pWorldResidents_STATIC_ACCESS = ptr;
	}
}