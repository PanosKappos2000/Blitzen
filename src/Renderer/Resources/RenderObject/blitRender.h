#pragma once
#include "Renderer/Resources/blitRenderingResources.h"

namespace BlitzenEngine
{
	struct RenderContainer
	{
		MeshTransform m_transforms[BlitzenCore::Ce_MaxRenderObjects ];
		uint32_t m_transformCount{ 0 };
		uint32_t m_staticTransformOffset{ BlitzenCore::Ce_MaxDynamicObjectCount };
		uint32_t m_staticTransformCount{ 0 };
		uint32_t m_dynamicTransformCount{ 0 };

		BlitzenEngine::RenderObject m_renders[BlitzenCore::Ce_MaxRenderObjects];
		uint32_t m_renderCount{ 0 };

		BlitzenEngine::RenderObject m_transparentRenders[BlitzenCore::Ce_MaxTransparentRenderObjects];
		uint32_t m_transparentRenderCount{ 0 };

		BlitzenEngine::RenderObject m_onpcRenders[BlitzenCore::Ce_MaxONPC_Objects];
		uint32_t m_onpcRenderCount{ 0 };
	};

	// Takes a mesh id and adds a render object based on that ID and a transform
	bool CreateRenderObject(RenderContainer& context, MeshResources& meshes, uint32_t transformId, uint32_t surfaceId);

	uint32_t CreateRenderObjectFromMesh(RenderContainer& context, MeshResources& meshes, uint32_t meshId, const BlitzenEngine::MeshTransform& transform, bool isDynamic);

	void CreateSingleRender(RenderContainer& context, MeshResources& meshes, const char* meshName, float scale);

	void LoadGeometryStressTest(RenderContainer& renders, MeshResources& meshContext, float transformMultiplier);

	void RandomizeTransform(MeshTransform& transform, float multiplier, float scale);

	void CreateRenderObjectWithRandomTransform(uint32_t meshId, RenderContainer& renders, MeshResources& meshContext, float randomTransformMultiplier, float scale);

	void CreateObliqueNearPlaneClippingTestObject(RenderContainer& renders, MeshResources& meshContext);
}