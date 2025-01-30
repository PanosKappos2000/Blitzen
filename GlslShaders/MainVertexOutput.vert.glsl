#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_NV_gpu_shader5 : enable
#extension GL_KHR_vulkan_glsl : enable

struct Vertex
{
    vec3 position;
    float16_t uvX, uvY;
    uint8_t normalX, normalY, normalZ;
};

layout(std430, binding = 0) readonly buffer VertexBuffer
{
    Vertex vertices[];
}vertexBuffer;

void main()
{
    Vertex currentVertex = vertexBuffer.vertices[gl_VertexIndex];
    gl_Position = vec4(currentVertex.position, 1);
}