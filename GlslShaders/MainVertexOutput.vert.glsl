#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_NV_uniform_buffer_std430_layout : enable

// Vertex position attribute slot
layout (location = 0) in vec3 vertexPos;

layout (location = 1) in float16_t textureMapU;

layout (location = 2) in float16_t texureMapV;

layout (location = 3) in uint8_t normalX;

layout(location = 4) in uint8_t normalY;

layout(location = 5) in uint8_t normalZ;

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

// This holds global shader data passed to the GPU at the start of each frame
layout(std430, binding = 0) uniform ShaderData
{
    // This is the result of the mulitplication of projection * view, to avoid calculating for every vertex shader invocation
    mat4 projectionView;
    mat4 view;

    vec4 sunColor;
    vec3 sunDir;

    vec3 viewPosition;// Needed for lighting calculations
}shaderData;

// Sends the normal to the fragment shader
layout (location = 0) out vec3 fragNormal;

// This function is used in every vertex shader invocation to give the object its orientation
vec3 RotateQuat(vec3 v, vec4 quat)
{
	return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

void main()
{
    Transform transform = transformBuffer.transforms[gl_DrawIDARB];

    gl_Position = shaderData.projectionView * vec4(RotateQuat(
    vertexPos, transform.orientation) * transform.scale + transform.pos, 1);

    vec3 normal = vec3(normalX, normalY, normalZ);
    fragNormal = RotateQuat(normal, transform.orientation);
}