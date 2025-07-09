#pragma once 
#include "blitCollision.h"
#include "Renderer/Resources/blitShaderShared.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_WORLD_BOUNDING_SPHERE_COUNT = BLIT_MAX_WORLD_RENDERS;
	constexpr uint32_t CE_MAX_WORLD_COLLIDER_COUNT = CE_MAX_WORLD_BOUNDING_SPHERE_COUNT;

	// Had some constructor trouble with unions that is why this struct is ugly as hell
    struct Collider
    {
        ColliderType type;
		BlitML::vec3 CAPSULEPONE_AABBMIN_SPHEREC;
		BlitML::vec3 CAPSULEPTWO_AABBMAX;
		float CAPSULERAD_SPHERERAD;
    };
    static_assert(sizeof(Collider) % 16 == 0);

	class ColliderContainer
	{
	public:
		BoundingSphere m_boundingSpheres[CE_MAX_WORLD_BOUNDING_SPHERE_COUNT];

		Collider m_colliders[CE_MAX_WORLD_COLLIDER_COUNT];// Static objects have set reactions

		// Adds bounding sphere for render object. If the render object is static, the sphere is pre transformed
		void AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform, uint32_t renderObjectID, RENDER_OBJECT_TYPE objectType);
	};
}