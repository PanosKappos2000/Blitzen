#pragma once
#include "Renderer/Resources/blitRenderingResources.h"

namespace BlitzenEngine
{
	enum RENDER_OBJECT_RES : int8_t
	{

	};

	enum RENDER_OBJECT_TYPE : uint8_t
	{
		OPAQUE = 0,
		OPAQUE_DYNAMIC = 1, 

		TRANSPARENT = 2,
		TRANSPARENT_DYNAMIC = 3
	};

	struct RENDER_OBJECT_CREATE_CONTEXT
	{
		RENDER_OBJECT_TYPE m_type;
		MeshTransform* pTransform{ nullptr };
		PrimitiveSurface* pSurface{ nullptr };
	};

	struct RenderContainer
	{
		MeshTransform m_transforms[BlitzenCore::Ce_MaxRenderObjects ];
		uint32_t m_transformCount{ 0 };
		uint32_t m_staticTransformOffset{ BlitzenCore::Ce_MaxDynamicObjectCount};
		uint32_t m_staticTransformCount{ 0 };
		uint32_t m_dynamicTransformCount{ 0 };

		BlitzenEngine::RenderObject m_renders[BlitzenCore::Ce_MaxRenderObjects];
		uint32_t m_renderCount{ 0 };

		BlitzenEngine::RenderObject m_transparentRenders[BlitzenCore::Ce_MaxTransparentRenderObjects];
		uint32_t m_transparentRenderCount{ 0 };

		RENDER_OBJECT_RES CreateRenderObject(RENDER_OBJECT_CREATE_CONTEXT& context);
	};

	// Takes a mesh id and adds a render object based on that ID and a transform
	bool CreateRenderObject(RenderContainer& context, MeshResources& meshes, uint32_t transformId, uint32_t surfaceId);

	uint32_t CreateRenderObjectFromMesh(RenderContainer& context, MeshResources& meshes, uint32_t meshId, const BlitzenEngine::MeshTransform& transform, bool isDynamic);

	void CreateSingleRender(RenderContainer& context, MeshResources& meshes, const char* meshName, float scale);

	void RandomizeTransform(MeshTransform& transform, float multiplier, float scale);

	void CreateRenderObjectWithRandomTransform(uint32_t meshId, RenderContainer& renders, MeshResources& meshContext, float randomTransformMultiplier, float scale);

	void CreateObliqueNearPlaneClippingTestObject(RenderContainer& renders, MeshResources& meshContext);
}