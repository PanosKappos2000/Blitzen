#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/HlslShaders/Headers/cpuShared.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_DYNAMIC_TRANSFORM_OFFSET = 0;
	constexpr uint32_t CE_STATIC_TRANSFORM_OFFSET = BlitzenCore::Ce_MaxWorldMovingResidentCount;
	constexpr uint32_t CE_MAX_STATIC_TRANSFORM_COUNT = BLIT_MAX_WORLD_TRANSFORM_COUNT - BlitzenCore::Ce_MaxWorldMovingResidentCount;

	enum class WorldTransformType: uint8_t
	{
		DYNAMIC = 0,
		STATIC = 1
	};

	struct TRANSFORM_CREATE_CONTEXT
	{
		WorldTransformType m_type{ WorldTransformType::STATIC };
		MeshTransform* m_pTransform{ nullptr };
		float m_scale{ 0.f };
		float m_randomTransformMultiplier{ 0.f };
	};

	class WorldTransformContainer
	{
	public:

		MeshTransform m_transforms[BLIT_MAX_WORLD_TRANSFORM_COUNT];
		uint32_t m_transformCount{ 0 };
		uint32_t m_dynamicTransformCount{ 0 };
		uint32_t m_staticTransformCount{ 0 };

		// Creates transform and returns its index in the transform list
		uint32_t CreateTransform(const TRANSFORM_CREATE_CONTEXT& transform);
	};

	void RandomizeTransform(MeshTransform& transform, float multiplier, float scale);
}