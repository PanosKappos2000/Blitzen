#version 450

#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

#define GRAPHICS_PIPELINE 

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

// All the values needed by the fragment shader
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

    // Calculate the model position by using the current transform data(the model position will be passed to the fragment shader and for gl_position)
    vec3 modelPosition = RotateQuat(currentVertex.position, currentInstance.orientation) * currentInstance.scale + currentInstance.pos;
    // Calculate final gl_position by projecting model position to clip coordinates
    gl_Position = shaderData.projectionView * vec4(modelPosition, 1.0);

    // Create a vec2 from the uvMap halfFloats to be passed to the fragment shader
    outUv = vec2(float(currentVertex.uvX), float(currentVertex.uvY));

    outMaterialTag = bufferAddrs.surfaceBuffer.surfaces[currentObject.surfaceId].materialTag;
    
    // Create a surface normal from the uint8s loaded for normals
    vec3 normal = vec3(currentVertex.normalX, currentVertex.normalY, currentVertex.normalZ);
    // Pass the normal after promoting it to model coordinates
    outNormal =  RotateQuat(normal, currentInstance.orientation);

    // Pass the model position, used for lighing calculations
    outModel = modelPosition;
}