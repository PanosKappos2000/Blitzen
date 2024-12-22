#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE

#include "ShaderBuffers.glsl.h"

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;


void main()
{
    uint threadIndex = gl_LocalInvocationID.x;
	uint groupIndex = gl_WorkGroupID.x;
	uint objectIndex = gl_GlobalInvocationID.x;

    IndirectOffsets currentRead = bufferAddrs.indirectBuffer.offsets[objectIndex];
    RenderObject currentObject = bufferAddrs.renderObjects.renderObjects[objectIndex];

    vec3 center = currentObject.center * currentObject.scale + currentObject.pos;
	float radius = currentObject.radius * currentObject.scale;
	bool visible = true;
	for (int i = 0; i < 6; ++i)
		visible = visible && dot(shaderData.frustumData[i], vec4(center, 1)) > -radius;
    
    
    if(visible)
    {
        // The level of detail index that should be used is derived by the distance fromt he camera
        float lodDistance = log2(max(1, (distance(center, vec3(shaderData.viewPosition)) - radius)));
	    uint lodIndex = clamp(int(lodDistance), 0, int(currentRead.lodCount) - 1);

        MeshLod currentLod = currentRead.lod[lodIndex];
        
        uint drawIndex = atomicAdd(bufferAddrs.indirectCount.drawCount, 1);
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].objectId = objectIndex;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].indexCount = currentLod.indexCount;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].instanceCount = 1;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstIndex = currentLod.firstIndex;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].vertexOffset = currentRead.vertexOffset;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstInstance = 0;
    }
    
    
    
}