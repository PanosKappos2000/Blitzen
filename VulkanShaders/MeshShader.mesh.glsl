#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_NV_mesh_shader: require

// TODO: bad for perf! local_size_x should be 32
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 64, max_primitives = 42) out;

#include "ShaderBuffers.glsl.h"

layout(location = 0) out vec2 outUv[];
layout(location = 1) out vec3 outNormal[];
layout(location = 2) out uint outMaterialTag[];
layout(location = 3) out vec3 outView[];
layout(location = 4) out vec3 outModel[];

void main()
{
    uint meshIndex = gl_WorkGroupID.x;

    // TODO: really bad for perf; our workgroup has 1 thread!
	for (uint i = 0; i < uint(bufferAddrs.meshBuffer.meshlets[meshIndex].vertexCount); ++i)
	{
		uint vi = bufferAddrs.meshBuffer.meshlets[meshIndex].vertices[i];
		vec3 position = vec3(bufferAddrs.vertexBuffer.vertices[vi].position);
		vec3 normal = vec3(bufferAddrs.vertexBuffer.vertices[vi].normal);
		vec2 texcoord = vec2(bufferAddrs.vertexBuffer.vertices[vi].uvX, bufferAddrs.vertexBuffer.vertices[vi].uvY);
		gl_MeshVerticesNV[i].gl_Position = shaderData.projectionView * vec4(position, 1.0);
	}

    gl_PrimitiveCountNV = uint(bufferAddrs.meshBuffer.meshlets[meshIndex].indexCount) / 3;
	// TODO: really bad for perf; our workgroup has 1 thread!
	for (uint i = 0; i < uint(bufferAddrs.meshBuffer.meshlets[meshIndex].indexCount); ++i)
	{
		// TODO: possibly bad for perf, consider writePackedPrimitiveIndices4x8NV
		gl_PrimitiveIndicesNV[i] = uint(bufferAddrs.meshBuffer.meshlets[meshIndex].indices[i]);
	}
}