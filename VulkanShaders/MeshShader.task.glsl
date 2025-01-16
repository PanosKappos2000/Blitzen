#version 450

#extension GL_EXT_mesh_shader: require
#extension GL_GOOGLE_include_directive: require

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

struct MeshTaskPayload
{
    uint clusterIndices[32];
};

taskPayloadSharedEXT MeshTaskPayload payload;

bool coneCull(vec4 cone, vec3 view)
{
	return dot(cone.xyz, view) > cone.w;
}

void main()
{

}