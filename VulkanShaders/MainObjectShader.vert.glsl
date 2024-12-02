#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

struct Vertices
{
    vec3 position;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertices vertices[];
};

layout(set = 0, binding = 0) uniform ShaderData
{
    mat4 projection;
    mat4 view;
    VertexBuffer vertexBuffer;
}shaderData;

void main()
{
    gl_Position = shaderData.projection * shaderData.view * vec4(shaderData.vertexBuffer.vertices[gl_VertexIndex].position, 1);
}