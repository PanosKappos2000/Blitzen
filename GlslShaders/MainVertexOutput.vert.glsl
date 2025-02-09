#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_NV_uniform_buffer_std430_layout : enable

struct Vertex
{
    vec3 position;
    float16_t uvX, uvY;
    uint8_t normalX, normalY, normalZ, normalW;
    uint8_t tangentX, tangentY, tangentZ, tangentW;
};

layout(std430, binding = 5) readonly buffer VertexBuffer
{
    Vertex vertices[];
}vertexBuffer;

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
    Vertex vertex = vertexBuffer.vertices[gl_VertexID];
    Transform transform = transformBuffer.transforms[gl_DrawIDARB];

    gl_Position = shaderData.projectionView * vec4(RotateQuat(
    vertex.position, transform.orientation) * transform.scale + transform.pos, 1);

    // Unpack surface normals
    vec3 normal = vec3(vertex.normalX, vertex.normalY, vertex.normalZ) / 127.0 - 1.0;
    // Pass the normal after promoting it to model coordinates
    fragNormal =  RotateQuat(normal, transform.orientation);
}