#pragma once

#ifdef __cplusplus
	using uint = uint32_t;
#endif

/****************************************************************************************************************************************************************************************
* THE MACROS IN THIS FILE ARE NOT TO BE CONVERTED TO CONSTEXPR AS THE FILE NEEDS TO BE INCLUDED IN SHADERS AS WELL																	    *
*****************************************************************************************************************************************************************************************/

#define BLIT_MAX_WORLD_RENDERS																5000000u
#define BLIT_MAX_WORLD_OPAQUE_STATIC_RENDERS												4500000u
#define BLIT_MAX_WORLD_TRANSPARENT_RENDERS													10000u
#define BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS												5000u
#define BLIT_TRANSPARENT_RENDER_OFFSET														0u
#define BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET													BLIT_MAX_WORLD_TRANSPARENT_RENDERS
#define BLIT_OPAQUE_STATIC_RENDER_OFFSET													15000
#ifdef __cplusplus
	static_assert(BLIT_OPAQUE_STATIC_RENDER_OFFSET == BLIT_MAX_WORLD_TRANSPARENT_RENDERS + BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS);
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

#define BLIT_OPAQUE_STATIC_CMD_COUNTER_OFFSET		0
#define BLIT_OPAQUE_DYNAMIC_CMD_COUNTER_OFFSET		1

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
	constexpr uint BLIT_HLSL_OCCDP_VISIBILITY_REGISTER = 10;
	constexpr uint BLIT_HLSL_INSTANCED_CMD_BUFFER_REGISTER = 11;
	constexpr uint BLIT_HLSL_INSTANCED_CMD_COUNTER_REGISTER = 12;
	constexpr uint BLIT_HLSL_HI_Z_OUTPUT_REGISTER = 13;

	constexpr uint BLIT_HLSL_RENDER_BUFFER_REGISTER = 0;
	constexpr uint BLIT_HLSL_BOUNDING_SPHERE_REGISTER = 1;
	constexpr uint BLIT_HLSL_SURFACE_BUFFER_REGISTER = 2;
	constexpr uint BLIT_HLSL_MOVEMENT_BUFFER_REGISTER = 3;
	constexpr uint BLIT_HLSL_LOD_BUFFER_REGISTER = 4;
	constexpr uint BLIT_HLSL_HI_Z_MAP_REGISTER = 5;
	constexpr uint BLIT_HLSL_CLUSTER_VTXS_REGISTER = 6;
	constexpr uint BLIT_HLSL_CLUSTER_SPHERES_REGISTER = 7;
	constexpr uint BLIT_HLSL_CLUSTER_CONES_REGISTER = 8;
	constexpr uint BLIT_HLSL_VTX_POSITIONS_REGISTER = 9;
	constexpr uint BLIT_HLSL_VTX_NORMALS_REGISTER = 10;
	constexpr uint BLIT_HLSL_VTX_TANGENTS_REGISTER = 11;
	constexpr uint BLIT_HLSL_VTX_TEXCOORDS_REGISTER = 12;
	constexpr uint BLIT_HLSL_MATERIAL_BUFFER_REGISTER = 13;
	constexpr uint BLIT_HLSL_HI_Z_INPUT_REGISTER = 14;
	constexpr uint BLIT_HLSL_INSTANCED_RENDERS_REGISTER = 15;
	constexpr uint BLIT_HLSL_TEXTURE_DESCRIPTORS_REGISTER = 16;

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

	constexpr uint BLIT_HLSL_TEX_SAMPLER_REGISTER = 0;
#endif

enum BLIT_RESIDENT_MOVEMENT_FLAG_BITS
{
	BLIT_RESIDENT_MOVEMENT_NONE = 0x0,
	BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT = 0x1,
	BLIT_RESIDENT_MOVEMENT_ROTATING_PITCH_BIT = 0x2,
	BLIT_RESIDENT_MOVEMENT_ROTATING_ROLL_BIT = 0x4
};

#ifdef __cplusplus
	enum BLITZEN_COLLISION_IDENTIFIER : uint
	{
		BLITZEN_COLLISION_FLAGS_BLOCK = 0,
		BLITZEN_COLLISION_FLAGS_WORLD_VARIABLE = 1
	};

	using WORLD_VARIABLE_IDENTIFIER = uint;
#endif