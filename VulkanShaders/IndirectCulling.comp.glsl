#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE
#define FRUSTUM_CULLING     true

#include "ShaderBuffers.glsl.h"

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint threadIndex = gl_LocalInvocationID.x;
	uint groupIndex = gl_WorkGroupID.x;
	uint objectIndex = gl_GlobalInvocationID.x;

    IndirectDraw currentRead = bufferAddrs.indirectBuffer.indirectDraws[objectIndex];
    RenderObject currentObject = bufferAddrs.renderObjects.renderObjects[objectIndex];

    vec3 center = currentObject.center * currentObject.scale + currentObject.pos;
	float radius = currentObject.radius * currentObject.scale;
	bool visible = true;
	for (int i = 0; i < 6; ++i)
		visible = visible && dot(shaderData.frustumData[i], vec4(center, 1)) > -radius;

    if(FRUSTUM_CULLING)
    {
        if(visible)
        {
            uint drawIndex = atomicAdd(bufferAddrs.indirectCount.drawCount, 1);
            bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].objectId = objectIndex;
            bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].indexCount = currentRead.indexCount;
            bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].instanceCount = 1;
            bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstIndex = currentRead.firstIndex;
            bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].vertexOffset = currentRead.vertexOffset;
            bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstInstance = 0;
        }
    }
    else
    {
        bufferAddrs.finalIndirectBuffer.indirectDraws[objectIndex].objectId = objectIndex;
        bufferAddrs.finalIndirectBuffer.indirectDraws[objectIndex].indexCount = currentRead.indexCount;
        bufferAddrs.finalIndirectBuffer.indirectDraws[objectIndex].instanceCount = 1;
        bufferAddrs.finalIndirectBuffer.indirectDraws[objectIndex].firstIndex = currentRead.firstIndex;
        bufferAddrs.finalIndirectBuffer.indirectDraws[objectIndex].vertexOffset = currentRead.vertexOffset;
        bufferAddrs.finalIndirectBuffer.indirectDraws[objectIndex].firstInstance = 0;
    }
}