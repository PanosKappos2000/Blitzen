#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

#include "ShaderBuffers.glsl.h"

layout(push_constant) uniform constants
{
    uint drawTag;
}PushConstants;

layout(location = 0) out vec2 outUv;
layout(location = 1) out uint outMaterialTag;

void main()
{
    Vertex currentVertex = shaderData.vertexBuffer.vertices[gl_VertexIndex];
    RenderObject renderObject = shaderData.renderObjects.renderObjects[PushConstants.drawTag];

    gl_Position = shaderData.projectionView * renderObject.worldMatrix * vec4(currentVertex.position, 1);

    outUv = vec2(currentVertex.uvX, currentVertex.uvY);
    outMaterialTag = renderObject.materialTag;
}