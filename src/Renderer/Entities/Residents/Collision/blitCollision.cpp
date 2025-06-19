#include "blitCollisionManager.h"
#include "Core/DbLog/blitLogger.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/blitMemory.h"

namespace BlitzenEngine
{
	uint32_t ColliderContainer::AddRenderObjectBoundingSphere(BoundingSphere* pSphere, MeshTransform& transform)
	{
		if (m_boundingSphereCount >= CE_MAX_WORLD_BOUNDING_SPHERE_COUNT)
		{
			BLIT_ERROR("%s: Reached maximum render object bounding sphere count", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
			return CE_MAX_WORLD_BOUNDING_SPHERE_COUNT;
		}

		auto& newcomer{ m_boundingSpheres[m_boundingSphereCount] };

		BlitzenCore::BlitMemCopy(&newcomer, pSphere, sizeof(BoundingSphere));

		newcomer.m_center = BlitML::RotateQuat(newcomer.m_center, transform.orientation) * transform.scale + transform.pos;
		newcomer.m_radius *= transform.scale;

		return m_boundingSphereCount++;
	}
}