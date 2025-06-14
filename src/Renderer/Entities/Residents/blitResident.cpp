#pragma once

#include "blitResidentManager.h"

namespace BlitzenEngine
{
	RESIDENT_CREATE_RES WORLD_RESIDENTS::AddResident(const RESIDENT_CREATE_CONTEXT& ctx)
	{
		RenderObject* pFirstRender{ nullptr };

		for (uint32_t prim = 0; prim < ctx.m_pResource->surfaceCount; ++prim)
		{
			RENDER_OBJECT_CREATE_CONTEXT renderContext{};
			renderContext.m_type = RENDER_OBJECT_TYPE::OPAQUE_STATIC;
			renderContext.m_primitiveID = prim + ctx.m_pResource->firstSurface;
			renderContext.m_transformID = m_transforms.CreateTransform(ctx.m_transformInfo);

			if (renderContext.m_transformID == BlitzenCore::Ce_MaxWorldTransformCount)
			{
				return WORLD_TRANSFORM_CREATION_FAILED;
			}

			uint32_t renderObjectId = m_renders.CreateRenderObject(renderContext);

			if (renderObjectId == BlitzenCore::Ce_MaxRenderObjectCount)
			{
				return RENDER_OBJECT_CREATION_FAILED;
			}

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
		BLIT_ASSERT_MESSAGE(pRender != nullptr, "Passed null render object");

		Resident res{};
		res.m_pRender = pRender;
		res.m_count = renderCount;

		return res;
	}
}