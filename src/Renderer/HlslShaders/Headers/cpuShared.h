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
#define BLIT_OPAQUE_STATIC_RENDER_OFFSET													BLIT_MAX_WORLD_TRANSPARENT_RENDERS + BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS
#define BLIT_MAX_WORLD_TRANSFORM_COUNT														BLIT_MAX_WORLD_RENDERS