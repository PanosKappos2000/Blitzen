#pragma once
#include "Renderer/Resources/blitRenderingResources.h"

namespace BlitzenEngine
{
	enum RENDER_OBJECT_RES : int8_t
	{
		SUCCESS = 0,

		CREATED_OPAQUE = 1,
		CREATED_OPAQUE_DYNAMIC = 2,
		CREATED_TRANSPARENT_DYNAMIC = 3,
		CREATED_TRANPARENT = 4,

		MAX_COUNT_EXCEEDED = -1,
	};

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

		// TODO: Should be decoupled
		void CreateTransform();
	};

	// Takes a mesh id and adds a render object based on that ID and a transform
	bool CreateRenderObject(RenderContainer& context, MeshResources& meshes, uint32_t transformId, uint32_t surfaceId);

	uint32_t CreateRenderObjectFromMesh(RenderContainer& context, MeshResources& meshes, uint32_t meshId, const BlitzenEngine::MeshTransform& transform, bool isDynamic);

	void CreateSingleRender(RenderContainer& context, MeshResources& meshes, const char* meshName, float scale);

	void RandomizeTransform(MeshTransform& transform, float multiplier, float scale);

	void CreateRenderObjectWithRandomTransform(uint32_t meshId, RenderContainer& renders, MeshResources& meshContext, float randomTransformMultiplier, float scale);

	void CreateObliqueNearPlaneClippingTestObject(RenderContainer& renders, MeshResources& meshContext);
}