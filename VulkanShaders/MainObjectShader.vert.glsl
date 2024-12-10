#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

#include "ShaderBuffers.glsl.h"

layout(push_constant) uniform constants
{
    uint drawTag;
}PushConstants;

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out uint outMaterialTag;
layout(location = 3) out vec3 outView;
layout(location = 4) out vec3 outModel;

void main()
{
    Vertex currentVertex = bufferAddrs.vertexBuffer.vertices[gl_VertexIndex];
    RenderObject renderObject = bufferAddrs.renderObjects.renderObjects[PushConstants.drawTag];

    vec4 modelPosition = renderObject.worldMatrix * vec4(currentVertex.position, 1);
    gl_Position = shaderData.projectionView * modelPosition;

    outUv = vec2(currentVertex.uvX, currentVertex.uvY);
    outMaterialTag = renderObject.materialTag;
    outNormal =  currentVertex.normal;
    outView = shaderData.viewPosition;
    outModel = modelPosition.xyz;
}