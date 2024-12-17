#version 450

#extension GL_GOOGLE_include_directive : require

#include "ShaderBuffers.glsl.h"

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint threadIndex = gl_LocalInvocationID.x;
	uint groupIndex = gl_WorkGroupID.x;
	uint drawIndex = groupIndex * 32 + threadIndex;

    IndirectDraw currentRead = bufferAddrs.indirectBuffer.indirectDraws[drawIndex];

    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].indexCount = currentRead.indexCount;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].instanceCount = currentRead.instanceCount;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstIndex = currentRead.firstIndex;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].vertexOffset = currentRead.vertexOffset;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstInstance = currentRead.firstInstance;
}