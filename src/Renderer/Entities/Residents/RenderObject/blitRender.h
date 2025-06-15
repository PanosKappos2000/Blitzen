#pragma once
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_WORLD_OPAQUE_RENDERS = 4'500'000;
	static_assert(CE_MAX_WORLD_OPAQUE_RENDERS < BlitzenCore::Ce_MaxRenderObjectCount);

	constexpr uint32_t CE_MAX_WORLD_TRANSPARENT_RENDERS = 500'000;
	static_assert(CE_MAX_WORLD_TRANSPARENT_RENDERS < BlitzenCore::Ce_MaxRenderObjectCount);

	static_assert(CE_MAX_WORLD_TRANSPARENT_RENDERS + CE_MAX_WORLD_OPAQUE_RENDERS <= BlitzenCore::Ce_MaxRenderObjectCount);

	constexpr uint32_t CE_OPAQUE_RENDER_OFFSET = 0;
	constexpr uint32_t CE_TRANSPARENT_RENDER_OFFSET = CE_MAX_WORLD_OPAQUE_RENDERS;

	enum RENDER_OBJECT_TYPE : uint8_t
	{
		OPAQUE_STATIC = 0,
		OPAQUE_DYNAMIC = 1, 

		TRANSPARENT_STATIC = 2,
		TRANSPARENT_DYNAMIC = 3
	};

	struct RENDER_OBJECT_CREATE_CONTEXT
	{
		RENDER_OBJECT_TYPE m_type{RENDER_OBJECT_TYPE::OPAQUE_STATIC};
		uint32_t m_transformID { 0 };
		uint32_t m_primitiveID { 0 };
	};

	struct RenderContainer
	{
		BlitzenEngine::RenderObject m_renders[BlitzenCore::Ce_MaxRenderObjectCount];

		uint32_t m_renderCount{ 0 };
		uint32_t m_transparentRenderCount{ 0 };
		uint32_t m_opaqueRenderCount{ 0 };

		uint32_t CreateRenderObject(RENDER_OBJECT_CREATE_CONTEXT& context);
	};
}