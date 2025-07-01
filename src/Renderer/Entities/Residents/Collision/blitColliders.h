#pragma once 
#include "blitCollision.h"
#include "Renderer/HlslShaders/Headers/cpuShared.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_WORLD_BOUNDING_SPHERE_COUNT = BLIT_MAX_WORLD_RENDERS;

	class ColliderContainer
	{
	public:
		BoundingSphere m_boundingSpheres[CE_MAX_WORLD_BOUNDING_SPHERE_COUNT];

		Collider m_colliders[BlitzenCore::Ce_MaxWorldVariableCount];// Static objects have set reactions

		// Adds bounding sphere for render object. If the render object is static, the sphere is pre transformed
		void AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform, uint32_t renderObjectID, bool isStatic);
	};
}