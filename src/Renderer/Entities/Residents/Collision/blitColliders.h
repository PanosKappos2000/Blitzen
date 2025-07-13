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

	struct ColliderFloat3
	{
		BlitML::vec3 data;// Can be used for CapsuleA, AABBMIN, AABBMax or sphere center.
	};

	struct ColliderFloat
	{
		float data; // Can be used for Capsule radius or Sphere radius.
	};

	class ColliderContainer
	{
	public:
		BoundingSphere m_boundingSpheres[CE_MAX_WORLD_BOUNDING_SPHERE_COUNT];
		ColliderFloat3 MColliderFloat3DataAMax[CE_MAX_WORLD_COLLIDER_COUNT];
		ColliderFloat3 MColliderFloat3DataBMin[CE_MAX_WORLD_COLLIDER_COUNT];
		ColliderFloat MColliderFloatData[CE_MAX_WORLD_COLLIDER_COUNT];
		ColliderType MColliderTypeData[CE_MAX_WORLD_COLLIDER_COUNT];
		uint32_t MWorldColliderCount{ 0 };

		// Non world variable residents are assumed to have predictable, blocking collision.
		// If something needs specialized collision, a world variable needs to be created
		// For example, a weapon or a bullet, and probably even destructibles need to be world variables
		// If this system becomes heavy, there could exist a resident that is one level below a world variable
		WVColliderResponse WVColliderHitData[BLIT_MAX_WORLD_VARIABLE_COUNT];

		// Possible different for event collisions
		BLIT_STRAIGHTHANDLE event_collision_placeholder_unused;

		// Adds bounding sphere for render object. If the render object is static, the sphere is pre transformed
		void AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform, uint32_t renderObjectID, RENDER_OBJECT_TYPE objectType);
	};
}