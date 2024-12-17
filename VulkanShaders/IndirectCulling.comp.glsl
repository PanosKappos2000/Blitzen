#version 450

#extension GL_GOOGLE_include_directive : require

#include "ShaderBuffers.glsl.h"

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint threadIndex = gl_LocalInvocationID.x;
	uint groupIndex = gl_WorkGroupID.x;
	uint drawIndex = gl_GlobalInvocationID.x;

    IndirectDraw currentRead = bufferAddrs.indirectBuffer.indirectDraws[drawIndex];

    uint bVisible = 1;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].indexCount = currentRead.indexCount;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].instanceCount = 1;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstIndex = currentRead.firstIndex;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].vertexOffset = currentRead.vertexOffset;
    bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstInstance = 0;
}