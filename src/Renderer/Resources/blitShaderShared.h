#pragma once

#ifdef __cplusplus
	#include <cstdint>
	using uint = uint32_t;
#endif

// TEMPORARILY DEFINING THESE HERE UNTIL I TUNE CMAKE OR BUILD A CUSTOM TOOL
#if !defined(NDEBUG)
#define BLITZEN_CONFIGURATION_ENGINE_DEV
#else 
#define BLITZEN_CONFIGURATION_GAME_EDITOR
#endif

#if defined(BLITZEN_CONFIGURATION_ENGINE_DEV) || defined(BLITZEN_CONFIGURATION_GAME_EDITOR)
#define BLIT_OFFLINE_BUILD
#define BLIT_VISUAL_DEBUG
#endif

#if defined(BLITZEN_CONFIGURATION_ENGINE_DEV) || defined(BLITZEN_CONFIGURATION_GAME_EDITOR) || defined(BLITZEN_CONFIGURATION_GAME_TEST)
#define BLIT_ASSERTIONS_ENABLED
#endif

#if defined(BLIT_OFFLINE_BUILD)
#define BLIT_OFFLINE_FUNC 
#else
#define BLIT_OFFLINE_FUNC [[deprecated("This function is for offline use only.")]]
#endif

#if defined(BLIT_VISUAL_DEBUG)
#define BLIT_VISUAL_DEBUG_FUNC
#else
#define BLIT_VISUAL_DEBUG_FUNC [[deprecated("Application should not ship with a visual debug function")]]
#endif

/****************************************************************************************************************************************************************************************
* THE MACROS IN THIS FILE ARE NOT TO BE CONVERTED TO CONSTEXPR AS THE FILE NEEDS TO BE INCLUDED IN SHADERS AS WELL																	    *
*****************************************************************************************************************************************************************************************/

#define BLIT_MAX_WORLD_VARIABLE_COUNT														5000u
#define BLIT_MAX_WORLD_RESIDENTS															5000000u
#define BLIT_MAX_WORLD_STATIC_RESIDENTS														4500000u
#ifdef __cplusplus
	static_assert(BLIT_MAX_WORLD_STATIC_RESIDENTS + BLIT_MAX_WORLD_VARIABLE_COUNT < BLIT_MAX_WORLD_RESIDENTS);
	constexpr bool IS_RESIDENT_STATIC(uint32_t resident) { return resident >= BLIT_MAX_WORLD_VARIABLE_COUNT; }
#endif
#define BLIT_MAX_WORLD_RENDERS																BLIT_MAX_WORLD_RESIDENTS
#define BLIT_MAX_WORLD_OPAQUE_STATIC_RENDERS												BLIT_MAX_WORLD_STATIC_RESIDENTS
#define BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS												5000u
#define BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET													0u
#ifdef __cplusplus
	static_assert(BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET == 0);
#endif
#define BLIT_OPAQUE_STATIC_RENDER_OFFSET													BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS
#define BLIT_MAX_WORLD_TRANSPARENTS															10000u
#define BLIT_TRANSPARENT_RENDER_OFFSET														4750000u
#ifdef __cplusplus
	static_assert(BLIT_TRANSPARENT_RENDER_OFFSET > BLIT_MAX_WORLD_STATIC_RESIDENTS + BLIT_OPAQUE_STATIC_RENDER_OFFSET);
#endif
#define BLIT_MAX_WORLD_TRANSFORM_COUNT														BLIT_MAX_WORLD_RENDERS

#define BLIT_MAX_STATIC_DRAW_COMMANDS														500000u
#ifdef __cplusplus
	static_assert(BLIT_MAX_STATIC_DRAW_COMMANDS <= BLIT_MAX_WORLD_OPAQUE_STATIC_RENDERS);
#endif

#define BLIT_MAX_DYNAMIC_DRAW_COMMANDS														5000u
#ifdef __cplusplus
	static_assert(BLIT_MAX_DYNAMIC_DRAW_COMMANDS <= BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS);
#endif

#define BLIT_MAX_INSTANCED_RENDERS															1000000u
#define BLIT_MINIMUM_INSTANCE_COUNT_THRESHOLD												10000u

#define BLIT_MAX_CLUSTER_GROUPS																1000000u
#define BLIT_MAX_CLUSTERS_PER_GROUP															64u
#define BLIT_MAX_CLUSTER_DRAW_COMMANDS														500000u
#ifdef __cplusplus
	static_assert(BLIT_MAX_CLUSTER_DRAW_COMMANDS <= BLIT_MAX_CLUSTER_GROUPS);
#endif

#define BLIT_MAX_WORLD_TEXTURE_RESOURCES													5000u
#define BLIT_BLANK_MATERIAL_INDEX															0u

#define BLIT_COLLISION_GRID_EXTENT															3200u
#define BLIT_COLLISION_GRID_CELL_EXTENT														64u
#define BLIT_COLLISION_GRID_CELL_FLAT_COUNT													(BLIT_COLLISION_GRID_EXTENT / BLIT_COLLISION_GRID_CELL_EXTENT)
#ifdef __cplusplus
	static_assert(BLIT_COLLISION_GRID_CELL_FLAT_COUNT == BLIT_COLLISION_GRID_EXTENT / BLIT_COLLISION_GRID_CELL_EXTENT);
#endif
#define BLIT_COLLISION_GRID_CELL_COUNT														(BLIT_COLLISION_GRID_CELL_FLAT_COUNT * BLIT_COLLISION_GRID_CELL_FLAT_COUNT)
#ifdef __cplusplus
	static_assert(BLIT_COLLISION_GRID_CELL_COUNT == BLIT_COLLISION_GRID_CELL_FLAT_COUNT * BLIT_COLLISION_GRID_CELL_FLAT_COUNT);
#endif

#define BLIT_COLLIDER_SCALE_MULTIPLIER														1.f
#define BLIT_COLLIDER_COUNT_PER_GRID_CELL													(BLIT_COLLISION_GRID_CELL_EXTENT / BLIT_COLLIDER_SCALE_MULTIPLIER)
#define BLIT_AVAILABLE_COLLIDER_SPACES														(BLIT_COLLIDER_COUNT_PER_GRID_CELL * BLIT_COLLISION_GRID_CELL_COUNT)
#ifdef __cplusplus
	static_assert(BLIT_COLLIDER_COUNT_PER_GRID_CELL == BLIT_COLLISION_GRID_CELL_EXTENT / BLIT_COLLIDER_SCALE_MULTIPLIER);
	static_assert(BLIT_AVAILABLE_COLLIDER_SPACES == (BLIT_COLLIDER_COUNT_PER_GRID_CELL * BLIT_COLLISION_GRID_CELL_COUNT));
#endif

#define BLIT_DYNAMIC_COLLIDER_COUNT_PER_GRID_CELL											(BLIT_COLLISION_GRID_CELL_EXTENT / BLIT_COLLIDER_SCALE_MULTIPLIER)
#define BLIT_AVAILABLE_DYNAMIC_COLLIDER_SPACES												(BLIT_DYNAMIC_COLLIDER_COUNT_PER_GRID_CELL * BLIT_COLLISION_GRID_CELL_COUNT)
#ifdef __cplusplus
		static_assert(BLIT_DYNAMIC_COLLIDER_COUNT_PER_GRID_CELL == BLIT_COLLISION_GRID_CELL_EXTENT / BLIT_COLLIDER_SCALE_MULTIPLIER);
		static_assert(BLIT_AVAILABLE_DYNAMIC_COLLIDER_SPACES == (BLIT_DYNAMIC_COLLIDER_COUNT_PER_GRID_CELL * BLIT_COLLISION_GRID_CELL_COUNT));
#endif

#define BLIT_TERRAIN_GRID_SIZE_TEMP															64
#define BLIT_MAX_HEIGHT_MAP_DATA_COUNT														(BLIT_TERRAIN_GRID_SIZE_TEMP * BLIT_TERRAIN_GRID_SIZE_TEMP)
#ifdef __cplusplus
		static_assert(BLIT_MAX_HEIGHT_MAP_DATA_COUNT == BLIT_TERRAIN_GRID_SIZE_TEMP * BLIT_TERRAIN_GRID_SIZE_TEMP);
#endif

#define BLIT_GRAVITATIONAL_ACCELERATION														0.1f
#define BLIT_TERRAIN_HEIGHT_TEST_VALUE														0.f

#define BLIT_HLSL_COLLIDER_RESOURCE_OFFSET													0

/************************************************************************************************************************************************
* DX12 REGISTERS																																*
*************************************************************************************************************************************************/
#ifdef __cplusplus
	constexpr uint BLIT_HLSL_OPAQUE_STATIC_CMD_BUFFER_REGISTER = 0;
	constexpr uint BLIT_HLSL_OPAQUE_STATIC_CMD_COUNTER_REGISTER = 1;
	constexpr uint BLIT_HLSL_OPAQUE_DYNAMIC_CMD_BUFFER_REGISTER = 2;
	constexpr uint BLIT_HLSL_OPAQUE_DYNAMIC_CMD_COUNTER_REGISTER = 3;
	constexpr uint BLIT_HLSL_SHADER_TRANSFORM_BUFFER_REGISTER = 4;
	constexpr uint BLIT_HLSL_CLUSTER_GROUP_BUFFER_REGISTER = 5;
	constexpr uint BLIT_HLSL_CLUSTER_COUNTER_BUFFER_REGISTER = 6;
	constexpr uint BLIT_HLSL_CLUSTER_VISIBILITY_BUFFER_REGISTER = 7;
	constexpr uint BLIT_HLSL_CLUSTER_CMD_BUFFER_REGISTER = 8;
	constexpr uint BLIT_HLSL_CLUSTER_CMD_COUNTER_REGISTER = 9;
	constexpr uint BLIT_HLSL_MAX_CLUSTER_COUNT_REIGSTER = 10; // Compute
	constexpr uint BLIT_HLSL_OCCDP_VISIBILITY_REGISTER = 11;
	constexpr uint BLIT_HLSL_VISIBLE_COUNTER_REGISTER = 12;// Compute + vertex
	constexpr uint BLIT_HLSL_HI_Z_OUTPUT_REGISTER = 13;// Compute
	constexpr uint BLIT_HLSL_WVTRANSFORM_BUFFER_REGISTER = 14;// Compute
	constexpr uint BLIT_HLSL_GRID_CELLS_REGISTER = 15;
	constexpr uint BLIT_HLSL_COLLIDER_IDXs_REGISTER = 16;
	constexpr uint BLIT_HLSL_TRANSFORMED_COLLIDER_AMAXRAD_REGISTER = 17;
	constexpr uint BLIT_HLSL_TRANSFORMED_COLLIDER_BMINTYPE_REGISTER = 18;
	constexpr uint BLIT_HLSL_GLOBAL_COLLIDER_IDXs_OFFSET_REGISTER = 19;
	constexpr uint BLIT_HLSL_INSTANCED_CMD_BUFFER_REGISTER = 20;
	constexpr uint BLIT_HLSL_INSTANCED_CMD_COUNTER_REGISTER = 21;
	constexpr uint BLIT_HLSL_COLLIDER_DEBUG_DRAW_CMD = 22;
	constexpr uint BLIT_HLSL_COLLIDER_DEBUG_DRAW_CMD_COUNTER = 23;

	constexpr uint BLIT_HLSL_RENDER_BUFFER_REGISTER = 0;// Compute + Vertex
	constexpr uint BLIT_HLSL_BOUNDING_SPHERE_REGISTER = 1;// Compute 
	constexpr uint BLIT_HLSL_SURFACE_BUFFER_REGISTER = 2;// Compute + Vertex
	constexpr uint BLIT_HLSL_TERRAIN_HEIGHT_BUFFER_REGISTER = 3;// Compute(might remove from shaders)
	constexpr uint BLIT_HLSL_LOD_BUFFER_REGISTER = 4;// Compute
	constexpr uint BLIT_HLSL_HI_Z_MAP_REGISTER = 5;// Compute
	constexpr uint BLIT_HLSL_CLUSTER_VTXS_REGISTER = 6;// Compute
	constexpr uint BLIT_HLSL_CLUSTER_SPHERES_REGISTER = 7;// Compute
	constexpr uint BLIT_HLSL_CLUSTER_CONES_REGISTER = 8;// Compute
	constexpr uint BLIT_HLSL_VTX_POSITIONS_REGISTER = 9;// Vertex
	constexpr uint BLIT_HLSL_VTX_NORMALS_REGISTER = 10;// Vertex
	constexpr uint BLIT_HLSL_VTX_TANGENTS_REGISTER = 11;// Vertex
	constexpr uint BLIT_HLSL_VTX_TEXCOORDS_REGISTER = 12;// Vertex
	constexpr uint BLIT_HLSL_MATERIAL_BUFFER_REGISTER = 13;// Pixel
	constexpr uint BLIT_HLSL_HI_Z_INPUT_REGISTER = 14;// Compute
	constexpr uint BLIT_HLSL_INSTANCED_RENDERS_REGISTER = 15;// Compute
	constexpr uint BLIT_HLSL_TERRAIN_VERTEX_POSITIONS_REGISTER = 16;// Vertex
	constexpr uint BLIT_HLSL_COLLIDER_AMAXRAD_REGISTER = 17;// Compute + Vertex(debug)
	constexpr uint BLIT_HLSL_COLLIDER_BMINTYPE_REGISTER = 18;// Compute + Vertex(debug)
	constexpr uint BLIT_HLSL_GRID_CELL_VISUAL_DEBUG_QUAD_REGISTER = 19; // Vetex(debug)
	constexpr uint BLIT_HLSL_TEXTURE_DESCRIPTORS_REGISTER = 20;// Vertex NOTE: Should always be the last register

	constexpr uint BLIT_HLSL_VIEW_DATA_REGISTER = 0;
	constexpr uint BLIT_HLSL_OPAQUE_STATIC_COUNT_CONSTANT_REGISTER = 1;
	constexpr uint BLIT_HLSL_OPAQUE_DYNAMIC_COUNT_CONSTANT_REGISTER = 2;
	constexpr uint BLIT_HLSL_OPAQUE_STATIC_OBJID_REGISTER = 3;
	constexpr uint BLIT_HLSL_OPAQUE_DYNAMIC_OBJID_REGISTER = 4;
	constexpr uint BLIT_HLSL_OPAQUE_INSTANCED_OBJID_REGISTER = 5;
	constexpr uint BLIT_HLSL_HI_Z_CONTANT_REGISTER = 6;
	constexpr uint BLIT_HLSL_CLUSTER_OBJID_REGISTER = 7;
	constexpr uint BLIT_HLSL_CLUSTER_WORK_COUNT_CONSTANT_REGISTER = 8;
	constexpr uint BLIT_HLSL_INSTANCE_WORK_COUNT_REGISTER = 9;
	constexpr uint BLIT_HLSL_BMPR_COLLISION_WORK_COUNT_REGISTER = 10;
	constexpr uint BLIT_HLSL_BMPR_COLLISION_INDIRECT_CELL_IDX_REGISTER = 11;
	constexpr uint BLIT_HLSL_COLLIDER_DEBUG_CONSTANT_REGISTER = 12;

	constexpr uint BLIT_HLSL_TEX_SAMPLER_REGISTER = 0;

	constexpr uint BLIT_HLSL_BLITZEN_LOGO_TEX_REGISTER = 0;
#endif

enum BLIT_RESIDENT_MOVEMENT_FLAG_BITS
{
	BLIT_RESIDENT_MOVEMENT_NONE = 0,
	BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT = 1 << 0,
	BLIT_RESIDENT_MOVEMENT_ROTATING_PITCH_BIT = 1 << 1,
	BLIT_RESIDENT_MOVEMENT_ROTATING_ROLL_BIT = 1 << 2, 
	BLIT_RESIDENT_MOVEMENT_GRAVITY_BIT = 1 << 3,
	BLIT_RESIDENT_MOVEMENT_FALLING_BIT = 1 << 4,
	BLIT_RESIDENT_MOVEMENT_VELOCITY_BIT = 1 << 5,
	BLIT_RESIDENT_MOVEMENT_ROTATE_TO_DIRECTION_BIT = 1 << 6,
};

enum BlitzenColliderType
{
	BlitzenColliderTypeSphere = 0,
	BlitzenColliderTypeAABB = 1,
	BlitzenColliderTypeCapsule = 2,
	BlitzenColliderTypeMax = 3,
};

enum BlitzenWorldVariableDataFlags : uint64_t
{
	BlitzenWorldVariableCollisionFlag = 1 << 0,
	BlitzenWorldVariableGravityFlag = 1 << 1,
	BlitzenWorldVariableFrameEventFlag = 1 << 2,
};

#ifdef __cplusplus
	static_assert(sizeof(BlitzenColliderType) == 4);
#endif

#define BMPR_DRIVE_BROAD_PHASE_COLLISION				0
#define BMPR_DRIVE_NARROW_PHASE_COLLISION				0
#ifdef __cplusplus
	#if BMPR_DRIVE_BROAD_PHASE_COLLISION == 0
		constexpr uint8_t BLITGCBroadPhaseCollisionBumper = 0;
	#else
		constexpr uint8_t BLITGCBroadPhaseCollisionBumper = 1;
	#endif
	#if BMPR_DRIVE_NARROW_PHASE_COLLISION == 0
		constexpr uint8_t BLITGCNarrowPhaseCollisionBumper = 0;
	#else 
			constexpr uint8_t BLITGCNarrowPhaseCollisionBumper = 1;
	#endif
	#if  defined(BLITZEN_CONFIGURATION_ENGINE_DEV) || (BMPR_DRIVE_NARROW_PHASE_COLLISION != 0)
		constexpr bool GCBlitGpuColliderFlag = 1;
	#else 
		constexpr bool GCBlitGpuCollidersFlag = 0;
	#endif
#endif