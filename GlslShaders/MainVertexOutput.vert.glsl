#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_NV_gpu_shader5 : enable
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_shader_draw_parameters : enable

layout (location = 0) in vec3 vertexPos;

struct Transform
{
    vec3 pos;
    float scale;
    vec4 orientation;
};

layout(std430, binding = 1) readonly buffer TransformBuffer
{
    Transform transforms[];
}transformBuffer;

uniform mat4 projectionView;

// This function is used in every vertex shader invocation to give the object its orientation
vec3 RotateQuat(vec3 v, vec4 quat)
{
	return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

void main()
{
    Transform transform = transformBuffer.transforms[0];

    gl_Position = projectionView * vec4(RotateQuat(
    vertexPos, transform.orientation) * transform.scale + transform.pos, 1);
}