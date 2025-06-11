#pragma once

#include "blitResident.h"
#include "blitWv.h"
#include "RenderObject/blitRender.h"

namespace BlitzenEngine
{
	struct WORLD_RESIDENTS
	{
		WV m_worldVariables[BlitzenCore::Ce_MaxWorldVariableCount];
		WVHOST m_worldVariableHost;

		Resident m_resident;

		RenderContainer m_renders;
	};
}