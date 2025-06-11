#pragma once

#include "Renderer/Resources/renderingResourcesTypes.h"
#include "BlitCL/blitStack.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_DYNAMIC_TRANSFORM_OFFSET = 0;

	enum class WorldTransformType: uint8_t
	{
		DYNAMIC = 0,
		STATIC = 1
	};

	class WorldTransformContainer
	{
		BlitCL::BlitStack<MeshTransform, BlitzenCore::Ce_MaxRenderObjects> m_transform;

		uint32_t m_transformCount{ 0 };

		uint32_t m_dynamicTransformOffset{ CE_DYNAMIC_TRANSFORM_OFFSET };
		uint32_t m_staticTransformOffset{ BlitzenCore::Ce_MaxMovingObjectCount };

		uint32_t m_dynamicTransformCount{ 0 };
		uint32_t m_staticTransformCount{ 0 };

		uint32_t CreateTransform(WorldTransformType type);
	};
}