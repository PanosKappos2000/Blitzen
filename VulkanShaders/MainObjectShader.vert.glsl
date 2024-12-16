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
layout(location = 3) out vec3 outModel;

void main()
{
    Vertex currentVertex = bufferAddrs.vertexBuffer.vertices[gl_VertexIndex];
    RenderObject renderObject = bufferAddrs.renderObjects.renderObjects[gl_DrawIDARB];

    // Rotating the model inside the shader seems like a waste but we'll see
    vec4 modelPosition = vec4(RotateQuat(currentVertex.position, renderObject.orientation) * renderObject.scale + renderObject.pos, 1.0);
    gl_Position = shaderData.projectionView * modelPosition;

    outUv = vec2(float(currentVertex.uvX), float(currentVertex.uvY));

    outMaterialTag = renderObject.materialTag;
    outNormal =  currentVertex.normal;
    outModel = modelPosition.xyz;
}