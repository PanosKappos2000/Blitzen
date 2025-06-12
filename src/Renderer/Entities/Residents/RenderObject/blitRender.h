#pragma once
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	enum RENDER_OBJECT_TYPE : uint8_t
	{
		OPAQUE_STATIC = 0,
		OPAQUE_DYNAMIC = 1, 

		TRANSPARENT_STATIC = 2,
		TRANSPARENT_DYNAMIC = 3
	};

	struct RENDER_OBJECT_CREATE_CONTEXT
	{
		RENDER_OBJECT_TYPE m_type;
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