#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

#define GRAPHICS_PIPELINE 

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out uint outMaterialTag;
layout(location = 3) out vec3 outModel;

void main()
{
    // Access the current vertex
    Vertex currentVertex = bufferAddrs.vertexBuffer.vertices[gl_VertexIndex];

    // Access the current object data
    RenderObject currentObject = bufferAddrs.objectBuffer.objects[bufferAddrs.indirectDrawBuffer.draws[gl_DrawIDARB].objectId];
    MeshInstance currentInstance = bufferAddrs.transformBuffer.instances[currentObject.meshInstanceId];

    vec3 modelPosition = RotateQuat(currentVertex.position, currentInstance.orientation) * currentInstance.scale + currentInstance.pos;
    gl_Position = shaderData.projectionView * vec4(modelPosition, 1.0);

    outUv = vec2(float(currentVertex.uvX), float(currentVertex.uvY));

    outMaterialTag = bufferAddrs.surfaceBuffer.surfaces[currentObject.surfaceId].materialTag;
    
    outNormal =  RotateQuat(currentVertex.normal, currentInstance.orientation);

    outModel = modelPosition;
}