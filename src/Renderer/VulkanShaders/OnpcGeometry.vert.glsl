#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_ARB_shader_draw_parameters : require

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"

layout(push_constant) uniform Constants
{
    mat4 onpcMat;
};

// All the values needed by the fragment shader
layout(location = 0) out vec2 outUv;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outTangent;
layout(location = 3) out uint outMaterialTag;
layout(location = 4) out vec3 outModel;

void main()
{
    // Accesses the current vertex
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];

    // Accesses the current object data
    RenderObject object = onpcReflectiveObjectBuffer.objects[indirectDrawBuffer.draws[gl_DrawIDARB].objectId];
    Transform transform = transformBuffer.instances[object.meshInstanceId];

    // Calculate the model position by using the current transform data(the model position will be passed to the fragment shader and for gl_position)
    vec3 modelPosition = RotateQuat(vertex.position, transform.orientation) * transform.scale + transform.pos;
    // Calculate final gl_position by projecting model position to clip coordinates
    gl_Position = onpcMat * viewData.view * vec4(modelPosition, 1.0);

    // Create a vec2 from the uvMap halfFloats to be passed to the fragment shader
    outUv = vec2(float(vertex.uvX), float(vertex.uvY));

    outMaterialTag = surfaceBuffer.surfaces[object.surfaceId].materialId;
    
    // Unpack surface normals
    vec3 normal = vec3(vertex.normalX, vertex.normalY, vertex.normalZ) / 127.0 - 1.0;
    // Pass the normal after promoting it to model coordinates
    outNormal =  RotateQuat(normal, transform.orientation);

    // Unpack tangents
    vec4 tangent = vec4(vertex.tangentX, vertex.tangentY, vertex.tangentZ, vertex.tangentW) / 127.0 - 1.0;
    tangent.xyz = RotateQuat(tangent.xyz, transform.orientation);
    outTangent = tangent;

    // Pass the model position, used for lighing calculations
    outModel = modelPosition;
}