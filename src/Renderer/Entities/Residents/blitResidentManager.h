#pragma once
#include "RenderObject/blitRender.h"
#include "RenderObject/worldTransform.h"
#include "Collision/blitColliders.h"

namespace BlitzenEngine
{
	constexpr uint32_t GCWorldVariableTransformErrorCode = BLIT_MAX_WORLD_VARIABLE_COUNT;

	using RESIDENT_CREATE_CONTEXT_FLAGS = uint64_t;
	enum RESIDENT_CREATE_FLAGS : uint64_t
	{
		RESIDENT_CREATE_BASIC = 1 << 0,
		RESIDENT_CREATE_MOVING = 1 << 1,
		RESIDENT_CREATE_NO_COLLISION = 1 << 2,
		RESIDENT_CREATE_WORLD_VARIABLE = 1 << 3,
	};

	enum class RESIDENT_CREATE_RES : int8_t
	{
		SUCCESS = 0,

		RENDER_OBJECT_CREATION_FAILED = -1,
		WORLD_TRANSFORM_CREATION_FAILED = -2,
		RESIDENT_CREATION_FAILED = -3,
		NO_WORLD_TRANSFORM_CONTEXT_GIVEN = -4,
		WORLD_VARIABLE_COUNT_EXCEEDED = -5,
		COLLIDER_CREATION_FAILED = -6,

		UNKNOWN = -10
	};

	inline const char* GET_RESIDENT_CREATE_RES_STRING(RESIDENT_CREATE_RES res)
	{
		switch (res)
		{
		case RESIDENT_CREATE_RES::SUCCESS: return "SUCCESS";
		case RESIDENT_CREATE_RES::RENDER_OBJECT_CREATION_FAILED: return "RENDER_OBJECT_CREATION_FAILED";
		case RESIDENT_CREATE_RES::WORLD_TRANSFORM_CREATION_FAILED: return "WORLD_TRANSFORM_CREATION_FAILED";
		case RESIDENT_CREATE_RES::RESIDENT_CREATION_FAILED: return "RESIDENT_CREATION_FAILED";
		case RESIDENT_CREATE_RES::WORLD_VARIABLE_COUNT_EXCEEDED: return "WORLD_VARIABLE_COUNT_EXCEEDED";
		case RESIDENT_CREATE_RES::COLLIDER_CREATION_FAILED: return "FAILED_TO_CREATE_RESIDENT_COLLIDER";
		default: case RESIDENT_CREATE_RES::UNKNOWN: return "UNKNOWN";
		}
	}

	struct RESIDENT_CREATE_CONTEXT
	{
		RESIDENT_CREATE_CONTEXT_FLAGS m_flags{ RESIDENT_CREATE_BASIC };
		uint32_t m_resourceID;
		TRANSFORM_CREATE_CONTEXT m_transformInfo{};
		BlitzenCore::FAT_BOOL m_isMoveable{ BlitzenCore::FAT_FALSE };
		WVTransform* pWorldVariableTransform{ nullptr };
	};

	struct WORLD_VARIABLE_CREATE_CONTEXT
	{
		RESIDENT_CREATE_CONTEXT residentCtx{};
		uint32_t m_worldVariableID{ 0 };
	};

	using WORLD_VARIABLE = uint32_t;
	using BEWV = WORLD_VARIABLE;

	class WORLD_RESIDENTS
	{
	public:
		// Everything that can be considered to have an impact on the world (rendering, gameplay, events) is a resident
		// A resident is an index that can access base components
		// The elements from 0 to BLIT_MAX_WORLD_VARIABLE_COUNT are assumed to be the world varialbes and should not have any static residents inside
		Resident mResidents[BLIT_MAX_WORLD_RESIDENTS];
		uint32_t mResidentCount{ 0 };
		RenderContainer m_renders;
		WorldTransformContainer mTransforms;
		ColliderContainer MColliders;

		// Every member variable with WV as a prefix is a world variable
		// World Variables are a subset of Blitzen World residents, that have unpredictable logic applied to them
		// Their components are meant to be highly compatible with shaders and they are designed around crowding
		WORLD_VARIABLE MWorldVariables[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		WVTransform WVTransforms[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		WVVelocity WVVelocityData[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		WVGravity WVGravityData[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		uint32_t mWorldVariableCount{ 0 };
		Resident WVWithGravityIDXs[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		uint32_t WVWithGravityCount{ 0 };
		Resident WVWithVelocity[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		uint32_t WVWithVelocityCount{ 0 };
		
		RESIDENT_CREATE_RES AddResident(const RESIDENT_CREATE_CONTEXT& ctx);
		RESIDENT_CREATE_RES AddWorldVariable(const WORLD_VARIABLE_CREATE_CONTEXT& ctx);

		void UpdateMovingResidents(float deltaTime);
		void UpdateFallingResidents(float deltaTime);

	private:
		// CPU side transform for world variables, since they need to be updated by game logic
		uint32_t CreateWorldVariableTransform(WVTransform* pWVTransform, float scale);
	};

	void InitializeWorldResidentsPointer_STATIC_ACCESS(WORLD_RESIDENTS* ptr);

	RESIDENT_CREATE_RES AddResident_STATIC_ACCESS(const RESIDENT_CREATE_CONTEXT& ctx);
}