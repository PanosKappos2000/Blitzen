#pragma once
#include "blitResident.h"
#include "RenderObject/blitRender.h"
#include "RenderObject/worldTransform.h"
#include "Collision/blitCollisionManager.h"

namespace BlitzenEngine
{
	using RESIDENT_CREATE_CONTEXT_FLAGS = int64_t;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_BASIC = 0;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_MOVING = 0x5;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_COLLISION = 0xA;

	enum RESIDENT_CREATE_RES : int8_t
	{
		SUCCESS = 0,

		RENDER_OBJECT_CREATION_FAILED = -1,
		WORLD_TRANSFORM_CREATION_FAILED = -2,
		RESIDENT_CREATION_FAILED = -3,
		NO_WORLD_TRANSFORM_CONTEXT_GIVEN = -4,

		UNKNOWN = -10
	};

	inline const char* GET_RESIDENT_CREATE_RES_STRING(RESIDENT_CREATE_RES res)
	{
		switch (res)
		{
		case RESIDENT_CREATE_RES::SUCCESS: return "SUCCESS";
		case RENDER_OBJECT_CREATION_FAILED: return "RENDER_OBJECT_CREATION_FAILED";
		case WORLD_TRANSFORM_CREATION_FAILED: return "WORLD_TRANSFORM_CREATION_FAILED";
		case RESIDENT_CREATION_FAILED: return "RESIDENT_CREATION_FAILED";
		default: case RESIDENT_CREATE_RES::UNKNOWN: return "UNKNOWN";
		}
	}

	struct RESIDENT_CREATE_CONTEXT
	{
		RESIDENT_CREATE_CONTEXT_FLAGS m_flags{ RESIDENT_CREATE_BASIC };
		Mesh* m_pResource{ nullptr };
		TRANSFORM_CREATE_CONTEXT m_transformInfo{};
		RENDER_OBJECT_TYPE* m_renderTypes{nullptr};
	};

	class WORLD_RESIDENTS
	{
	public:
		WVKEY m_worldVariableAccessors[BlitzenCore::Ce_MaxWorldVariableCount]{};
		uint32_t m_worldVariableCount{ 0 };
		Resident m_residents[BlitzenCore::Ce_MaxWorldResidentCount];
		uint32_t m_residentCount{ 0 };
		RenderContainer m_renders;
		WorldTransformContainer m_transforms;
		ColliderContainer m_colliders;

		RESIDENT_CREATE_RES AddResident(const RESIDENT_CREATE_CONTEXT& ctx);

		RESIDENT_CREATE_RES AddWVResident();
	};

	void SetWorldResidentsPtr_STATIC_ACCESS(WORLD_RESIDENTS* ptr);

	MeshTransform& GetMeshTransform_STATIC_ACCESS(uint32_t transformID);
}