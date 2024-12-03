#version 450

#extension GL_EXT_buffer_reference2 : require
#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

struct Vertex
{
    vec3 position;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

struct RenderObject
{
    mat4 worldMatrix;
};

layout(buffer_reference, std430) readonly buffer RenderObjectBuffer
{
    RenderObject renderObjects[];
};

layout(set = 0, binding = 0) uniform ShaderData
{
    mat4 projection;
    mat4 view;
    mat4 projectionView;

    VertexBuffer vertexBuffer;
    RenderObjectBuffer renderObjects;
}shaderData;

layout(push_constant) uniform constants
{
    uint drawTag;
}PushConstants;

void main()
{
    gl_Position = shaderData.projection * shaderData.view * shaderData.renderObjects.renderObjects[PushConstants.drawTag].worldMatrix *
    vec4(shaderData.vertexBuffer.vertices[gl_VertexIndex].position, 1);
}