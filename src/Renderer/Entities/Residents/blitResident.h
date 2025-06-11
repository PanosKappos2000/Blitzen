#pragma once

#include "Renderer/Resources/renderingResourcesTypes.h"

namespace BlitzenEngine
{
	struct Resident
	{
		BoundingSphere* m_pCollision{ nullptr };
		RenderObject* m_pRenderObject{ nullptr };
		uint32_t renderObjectCount;
		BlitzenEngine::MeshTransform* pTransform{ nullptr };
	};
}