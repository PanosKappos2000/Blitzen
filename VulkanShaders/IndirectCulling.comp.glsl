#version 450

#define NO_FRUSTUM_CULLING     false

#extension GL_GOOGLE_include_directive : require

#include "ShaderBuffers.glsl.h"

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint threadIndex = gl_LocalInvocationID.x;
	uint groupIndex = gl_WorkGroupID.x;
	uint drawIndex = gl_GlobalInvocationID.x;

    IndirectDraw currentRead = bufferAddrs.indirectBuffer.indirectDraws[drawIndex];
    RenderObject currentObject = bufferAddrs.renderObjects.renderObjects[drawIndex];

    vec3 center = currentObject.center * currentObject.scale + currentObject.pos;
	float radius = currentObject.radius * currentObject.scale;
	bool visible = true;
	for (int i = 0; i < 6; ++i)
		visible = visible && dot(shaderData.frustumData[i], vec4(center, 1)) > -radius;

    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].indexCount = currentRead.indexCount;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].instanceCount = (visible || NO_FRUSTUM_CULLING) ? 1 : 0;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstIndex = currentRead.firstIndex;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].vertexOffset = currentRead.vertexOffset;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstInstance = 0;
}