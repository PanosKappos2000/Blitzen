#pragma once
#include "blitResident.h"
#include "RenderObject/blitRender.h"
#include "RenderObject/worldTransform.h"
#include "Collision/blitColliders.h"

namespace BlitzenEngine
{
	using RESIDENT_CREATE_CONTEXT_FLAGS = int64_t;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_BASIC = 0;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_MOVING = 0x5;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_COLLISION = 0xA;
	constexpr RESIDENT_CREATE_CONTEXT_FLAGS RESIDENT_CREATE_WORLD_VARIABLE = 0xF;

	enum class RESIDENT_CREATE_RES : int8_t
	{
		SUCCESS = 0,

		RENDER_OBJECT_CREATION_FAILED = -1,
		WORLD_TRANSFORM_CREATION_FAILED = -2,
		RESIDENT_CREATION_FAILED = -3,
		NO_WORLD_TRANSFORM_CONTEXT_GIVEN = -4,
		WORLD_VARIABLE_COUNT_EXCEEDED = -5,

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
		default: case RESIDENT_CREATE_RES::UNKNOWN: return "UNKNOWN";
		}
	}

	struct RESIDENT_CREATE_CONTEXT
	{
		RESIDENT_CREATE_CONTEXT_FLAGS m_flags{ RESIDENT_CREATE_BASIC };
		uint32_t m_resourceID;
		TRANSFORM_CREATE_CONTEXT m_transformInfo{};
		BlitzenCore::FAT_BOOL m_isMoveable{ BlitzenCore::FAT_FALSE };
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
		Resident m_residents[BLIT_MAX_WORLD_RESIDENTS];
		uint32_t m_residentCount{ 0 };

		// Every member variable with WV as a prefix is a world variable
		// World Variables are a subset of Blitzen World residents, that have unpredictable logic applied to them
		// Their components are meant to be highly compatible with shaders and they are designed around crowding
		WORLD_VARIABLE MWorldVariables[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		uint32_t MWorldVariableCount{ 0 };
		WVGravity WVGravityData[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		uint32_t WVWithGravityIDXs[BLIT_MAX_WORLD_VARIABLE_COUNT]{};
		uint32_t WVWithGravityCount{ 0 };
		WVVelocity WVVelocityData[BLIT_MAX_WORLD_VARIABLE_COUNT]{};

		// The render is an index to the resource that will be used when rendering a resident(if the resident can be rendered)
		RenderContainer m_renders;
		WorldTransformContainer m_transforms;
		ColliderContainer MColliders;

		RESIDENT_CREATE_RES AddResident(const RESIDENT_CREATE_CONTEXT& ctx);

		RESIDENT_CREATE_RES AddWorldVariable(const WORLD_VARIABLE_CREATE_CONTEXT& ctx);

		void UpdateMovingResidents(float deltaTime);

		void UpdateFallingResidents(float deltaTime);
	};

	void InitializeWorldResidentsPointer_STATIC_ACCESS(WORLD_RESIDENTS* ptr);

	RESIDENT_CREATE_RES AddResident_STATIC_ACCESS(const RESIDENT_CREATE_CONTEXT& ctx);
}