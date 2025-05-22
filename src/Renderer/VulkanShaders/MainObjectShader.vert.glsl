#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

#define GRAPHICS_PIPELINE 
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

//#define MESH_TEST

// All the values needed by the fragment shader
layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outTangent;
layout(location = 3) out uint outMaterialTag;
layout(location = 4) out vec3 outModel;

layout(push_constant) uniform Constants
{
    RenderObjectBuffer renderObjects;
}rodvpc;

#ifndef MESH_TEST
void main()
{
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    RenderObject object = rodvpc.renderObjects.objects[indirectDrawBuffer.draws[gl_DrawIDARB].objectId];
    Transform transform = transformBuffer.instances[object.meshInstanceId];

    // Model position. Will be used for gl_position and exported to frag
    vec3 modelPosition = RotateQuat(vertex.position, transform.orientation) * transform.scale + transform.pos;
    gl_Position = viewData.projectionView * vec4(modelPosition, 1.0);
    outModel = modelPosition;

    // Uv map for frag
    outUv = vec2(vertex.uvX, vertex.uvY);
    // Material tag for frag
    outMaterialTag = surfaceBuffer.surfaces[object.surfaceId].materialId;
    
    // Unpacks surface normals for frag
    vec3 normal = vec3(vertex.normalX, vertex.normalY, vertex.normalZ) / 127.5 - 1.0;
    outNormal =  RotateQuat(normal, transform.orientation);

    // Unpacks tangents for frag
    vec4 tangent = vec4(vertex.tangentX, vertex.tangentY, vertex.tangentZ, vertex.tangentW) / 127.5 - 1.0;
    tangent.xyz = RotateQuat(tangent.xyz, transform.orientation);
    outTangent = tangent;
}
#else
void main()
{
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    Transform transform = transformBuffer.instances[object.meshInstanceId];

    vec3 modelPosition = RotateQuat(vertex.position, transform.orientation) * transform.scale + transform.pos;
    gl_Position = viewData.projectionView * vec4(modelPosition, 1.0);
}
#endif