#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_mesh_shader : require
#extension GL_ARB_shader_draw_parameters : require

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 64, max_primitives = 126) out;

#include "ShaderBuffers.glsl.h"

layout(location = 0) out vec2 outUv[];
layout(location = 1) out vec3 outNormal[];
layout(location = 2) out uint outMaterialTag[];
layout(location = 3) out vec3 outModel[];

//taskPayloadSharedEXT uint meshletIndices[32];

void main()
{
    uint threadId = gl_LocalInvocationID.x;
	//uint meshletId = meshletIndices[gl_WorkGroupID.x];

    //uint drawId
}