#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

#define GRAPHICS_PIPELINE 

#include "ShaderBuffers.glsl.h"

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out uint outMaterialTag;
layout(location = 3) out vec3 outModel;

void main()
{
    // Access the current vertex
    Vertex currentVertex = bufferAddrs.vertexBuffer.vertices[gl_VertexIndex];

    // Access the current object data
    RenderObject currentObject = bufferAddrs.objectBuffer.objects[bufferAddrs.finalIndirectBuffer.indirectDraws[gl_DrawIDARB].objectId];
    MeshInstance currentInstance = bufferAddrs.meshInstanceBuffer.instances[currentObject.meshInstanceId];
    Surface currentSurface = bufferAddrs.surfaceBuffer.surfaces[currentObject.surfaceId];

    vec4 modelPosition = vec4(RotateQuat(currentVertex.position, currentInstance.orientation) * currentInstance.scale + currentInstance.pos, 1.0);
    gl_Position = shaderData.projectionView * modelPosition;

    outUv = vec2(float(currentVertex.uvX), float(currentVertex.uvY));

    outMaterialTag = currentSurface.materialTag;
    
    outNormal =  currentVertex.normal;

    outModel = modelPosition.xyz;
}