#pragma once

#include "Renderer/Resources/renderingResourcesTypes.h"

namespace BlitzenEngine
{
	struct MeshPrimitiveContext
	{
		BlitzenCore::BIG_BOOL m_primitiveTransparencyFlags;

		uint32_t m_primitiveVertexCount;

		uint32_t m_primitiveVertexOffset;
	};

	struct MeshPrimitivesDataContainer
	{

	};
}