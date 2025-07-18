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
        BlitzenColliderType type;
		BlitML::vec3 CAPSULEPONE_AABBMIN_SPHEREC;
		BlitML::vec3 CAPSULEPTWO_AABBMAX;
		float CAPSULERAD_SPHERERAD;
    };
    static_assert(sizeof(Collider) % 16 == 0);

	class ColliderContainer
	{
	public:
		BoundingSphere m_boundingSpheres[CE_MAX_WORLD_BOUNDING_SPHERE_COUNT];
		ColliderAMaxRad MColliderAMaxRad[CE_MAX_WORLD_COLLIDER_COUNT];// Holds data for capsule A or AABB max on xyz, capsule or sphere radius on w
		ColliderBMinType MColliderBMinType[CE_MAX_WORLD_COLLIDER_COUNT];// Holds data for capsule B or AABB min on xyz, collider type on w
		uint32_t mStaticColliderCount{ 0 };

		// Non world variable residents are assumed to have predictable, blocking collision.
		// If something needs specialized collision, a world variable needs to be created
		// For example, a weapon or a bullet, and probably even destructibles need to be world variables
		// If this system becomes heavy, there could exist a resident that is one level below a world variable
		WVColliderResponse WVColliderHitData[BLIT_MAX_WORLD_VARIABLE_COUNT];
		ColliderAMaxRad MTransformedColliderAMaxRad[BLIT_MAX_WORLD_VARIABLE_COUNT];
		ColliderBMinType MTransformedColliderBMinType[BLIT_MAX_WORLD_VARIABLE_COUNT];
		uint32_t mWorldVariableColliderCount{ 0 };

		// Possible different for event collisions
		BLIT_STRAIGHTHANDLE event_collision_placeholder_unused;

		CollisionMessage* MCollsionMessage{ nullptr };
		uint32_t mCollisionMessageCount{ 0 };

		// Adds bounding sphere for render object. If the render object is static, the sphere is pre transformed
		void AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform, uint32_t renderObjectID, RENDER_OBJECT_TYPE objectType);

		bool LogResidentForCollision(Resident resident, SplitColliderDataPair& data, MeshTransform& residentTransform,
			WVColliderResponse behavior = {BlitzenCollisionFlagsBlock, BlitzenCollisionFlagsBlock});

		void TransformCollidersWithoutBMPR(Resident* movingResidentArr, uint32_t movingResidentCount, WVTransform* transformArr, MeshTransform* gpuTransformArr);

		void CheckCapsuleColliderInsideGridCell(Resident hitter, GridCellOffsets& cell, Resident* indices);

		void CheckAABBColliderInsideGridCell(Resident hitter, GridCellOffsets& cell, Resident* indices);

		void CheckSphereColliderInsideGridCell(Resident hitter, GridCellOffsets& cell, Resident* indices);
	};

	
}