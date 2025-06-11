#pragma once
#include "Renderer/Resources/renderingResourcesTypes.h"

namespace BlitzenEngine
{
	struct DynamicTransformShaderData
	{

	};

	struct DynamicTransform
	{
		BlitML::vec3 m_rotation{ 0.f };

		BlitML::vec3 m_velocity{ 0.f };

		uint32_t m_transformID{ BlitzenCore::Ce_MaxRenderObjects };

		uint32_t m_thisID{ BlitzenCore::Ce_MaxDynamicObjectCount };
		bool m_isBlock{ false };
		bool m_isMoving{ false };
	};

	void CreateDynamicTransform();

	void RotateEntity(DynamicTransform* pTransform, BlitML::fRotation& rotation, BlitML::float3& velocity, float deltaTime, uint32_t worldVariableID);
}