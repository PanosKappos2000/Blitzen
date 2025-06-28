#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Renderer/HlslShaders/Headers/cpuShared.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_ALLOWED_INSTANCED_RESOURCES = 10;

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

	class RenderContainer
	{
	public:

		RenderObject m_renders[BLIT_MAX_WORLD_RENDERS];
		BoundingSphere m_boundingSpheres[BLIT_MAX_WORLD_RENDERS];

		uint32_t m_opaqueDynamicCount{ 0 };
		uint32_t m_transparentStaticCount{ 0 };
		uint32_t m_opaqueStaticCount{ 0 };
		uint32_t RENDER_COUNT{ 0 };

		uint32_t CreateRenderObject(RENDER_OBJECT_CREATE_CONTEXT& context);

		InstancedRender* m_instancedRenderArrays;

		uint32_t CreateInstancedRenderObject(uint32_t* transformIDArr, uint32_t m_primitiveID, uint32_t count);
	};
}