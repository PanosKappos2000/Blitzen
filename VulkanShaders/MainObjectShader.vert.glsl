#version 450

#extension GL_EXT_buffer_reference2 : require
#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

struct Vertex
{
    vec3 position;
    vec2 uvMap;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

struct RenderObject
{
    mat4 worldMatrix;
    uint textureTag;
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

layout(location = 0) out vec2 outUv;
layout(location = 1) out uint outTextureTag;

void main()
{
    Vertex currentVertex = shaderData.vertexBuffer.vertices[gl_VertexIndex];
    RenderObject renderObject = shaderData.renderObjects.renderObjects[PushConstants.drawTag];

    gl_Position = shaderData.projectionView * renderObject.worldMatrix * vec4(currentVertex.position, 1);

    outUv = currentVertex.uvMap;
    outTextureTag = renderObject.textureTag;
}