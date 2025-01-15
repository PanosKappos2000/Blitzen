#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_mesh_shader : require
#extension GL_ARB_shader_draw_parameters : require

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 64, max_primitives = 126) out;

#include "ShaderBuffers.glsl.h"

layout(location = 0) out vec2 outUv[];
layout(location = 1) out vec3 outNormal[];
layout(location = 2) out uint outMaterialTag[];
layout(location = 3) out vec3 outModel[];

taskPayloadSharedEXT uint meshletIndices[32];

void main()
{
    uint threadId = gl_LocalInvocationID.x;
	uint meshletId = meshletIndices[gl_WorkGroupID.x];

    uint drawId = bufferAddrs.indirectTaskBuffer.tasks[gl_DrawIDARB].taskId;

    // Access the current object data
    RenderObject currentObject = bufferAddrs.objectBuffer.objects[drawId];
    MeshInstance currentInstance = bufferAddrs.meshInstanceBuffer.instances[currentObject.meshInstanceId];
    Surface currentSurface = bufferAddrs.surfaceBuffer.surfaces[currentObject.surfaceId];

    // Meshlet data
    uint vertexCount = uint(bufferAddrs.meshletBuffer.meshlets[meshletId].vertexCount);
	uint triangleCount = uint(bufferAddrs.meshletBuffer.meshlets[meshletId].triangleCount);

    uint dataOffset = bufferAddrs.meshletBuffer.meshlets[meshletId].dataOffset;
	uint vertexOffset = dataOffset;
	uint indexOffset = dataOffset + vertexCount;

    for(uint i = threadId; i < vertexCount; i += 64)
    {
        uint vertexIndex = 0;//meshletData[vertexOffset + i] + currentSurface.vertexOffset;
        Vertex currentVertex = bufferAddrs.vertexBuffer.vertices[vertexIndex];

        vec3 position = currentVertex.position;
		vec3 normal = currentVertex.normal;
		vec2 uv = vec2(currentVertex.uvX, currentVertex.uvY);

        gl_MeshVerticesEXT[i].gl_Position = shaderData.projectionView * 
        vec4(RotateQuat(position, currentInstance.orientation) * currentInstance.scale + currentInstance.pos, 1);
    }
}