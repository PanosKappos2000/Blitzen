#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_NV_mesh_shader: require
#extension GL_ARB_shader_draw_parameters : require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 64, max_primitives = 126) out;


#include "ShaderBuffers.glsl.h"

layout(location = 0) out vec2 outUv[];
layout(location = 1) out vec3 outNormal[];
layout(location = 2) out uint outMaterialTag[];
layout(location = 3) out vec3 outModel[];

void main()
{
    uint meshIndex = gl_WorkGroupID.x;
	uint triangleIndex = gl_LocalInvocationID.x;

	uint triangleCount = uint(bufferAddrs.meshBuffer.meshlets[meshIndex].triangleCount);
	uint vertexCount = uint(bufferAddrs.meshBuffer.meshlets[meshIndex].vertexCount);
	uint indexCount = triangleCount * 3;

	RenderObject renderObject = bufferAddrs.renderObjects.renderObjects[gl_DrawIDARB];

    // TODO: if we have meshlets with 62 or 63 vertices then we pay a small penalty for branch divergence here - we can instead redundantly xform the last vertex
	for (uint i = triangleIndex; i < vertexCount; i+=32)
	{
		// Vertex index
		uint vi = bufferAddrs.meshBuffer.meshlets[meshIndex].vertices[i];

		vec3 position = vec3(bufferAddrs.vertexBuffer.vertices[vi].position);
		vec3 normal = vec3(bufferAddrs.vertexBuffer.vertices[vi].normal);
		vec2 texcoord = vec2(bufferAddrs.vertexBuffer.vertices[vi].uvX, bufferAddrs.vertexBuffer.vertices[vi].uvY);

		vec4 modelPosition = vec4(RotateQuat(position, renderObject.orientation) * renderObject.scale + renderObject.pos, 1.0);
		gl_MeshVerticesNV[i].gl_Position = shaderData.projectionView * modelPosition;
		
	}

	for (uint i = triangleIndex; i < indexCount; i += 32)
	{
		gl_PrimitiveIndicesNV[i] = uint(bufferAddrs.meshBuffer.meshlets[meshIndex].indices[i]);
	}

	if(triangleIndex == 0)
	{
		gl_PrimitiveCountNV = triangleCount;
	}
}