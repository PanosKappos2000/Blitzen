#pragma once

#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	// Temp(this struct might not even be needed)
	struct Resident
	{
		RenderObject* m_pRender{ nullptr };
		uint32_t m_count{ 0 };
	};

	Resident CreateResident(RenderObject* pRender, uint32_t renderCount);
}