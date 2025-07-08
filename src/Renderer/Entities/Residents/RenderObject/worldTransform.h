#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/Resources/blitShaderShared.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_DYNAMIC_TRANSFORM_OFFSET = 0;
	constexpr uint32_t CE_STATIC_TRANSFORM_OFFSET = BLIT_MAX_WORLD_VARIABLE_COUNT;
	constexpr uint32_t CE_MAX_STATIC_TRANSFORM_COUNT = BLIT_MAX_WORLD_STATIC_RESIDENTS;
	constexpr uint32_t CE_TRANSPARENT_OFFSET = BLIT_TRANSPARENT_RENDER_OFFSET;
	constexpr uint32_t CE_TRANSFORM_CREATE_ERROR_CODE = BLIT_MAX_WORLD_TRANSFORM_COUNT;

	enum class WorldTransformType: uint8_t
	{
		DYNAMIC = 0,
		STATIC = 1,
		BOUND_TO_TRANSPARENT = 2
	};

	struct TRANSFORM_CREATE_CONTEXT
	{
		WorldTransformType m_type{ WorldTransformType::STATIC };
		MeshTransform* m_pTransform{ nullptr };
		CPU_TRANSFORM* cpu_pTransform{ nullptr };
	};

	class WorldTransformContainer
	{
	public:

		MeshTransform m_transforms[BLIT_MAX_WORLD_TRANSFORM_COUNT];
		uint32_t m_transformCount{ 0 };
		uint32_t m_staticTransformCount{ 0 };

		CPU_TRANSFORM m_moveables[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		uint32_t m_moveableCount{ 0 };
		BlitML::float3 m_velocities[BLIT_MAX_WORLD_VARIABLE_COUNT];
		BlitML::fRotation m_rotations[BLIT_MAX_WORLD_VARIABLE_COUNT];

		uint32_t m_transparentCount{ 0 };

		// Creates transform and returns its index in the transform list
		uint32_t CreateTransform(const TRANSFORM_CREATE_CONTEXT& transform);
	};

	void RandomizeTransform(MeshTransform* pTransform, float multiplier, float scale);
	void RandomizeTransform(CPU_TRANSFORM* pTransform, float multiplier);

	// UNSAFE!!!
	CPU_TRANSFORM& GetWorldTransform_STATIC_ACCESS(uint32_t residentID);
}