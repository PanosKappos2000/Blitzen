#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "blitWv.h"

namespace BlitzenEngine
{
	// Temp(this struct might not even be needed)
	struct Resident
	{
		RenderObject* m_pRender{ nullptr };
		uint32_t m_count{ 0 };

		WVKEY* m_pWorldVariableAccess{nullptr};
	};

	Resident CreateResident(RenderObject* pRender, uint32_t renderCount);
}