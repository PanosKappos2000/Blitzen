#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE

#include "ShaderBuffers.glsl.h"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (set = 0, binding = 2) uniform CullingData
{
    float proj0;
    float proj11;
    float zNear;

    // Occulusion culling depth pyramid data
    float pyramidWidth;
    float pyramidHeight;
}cullingData;

layout (set = 0, binding = 3) uniform sampler2D depthPyramid;

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x;

    RenderObject currentObject = bufferAddrs.renderObjects.renderObjects[objectIndex];
    uint currentVisibility = bufferAddrs.visibilityBuffer.visibilities[objectIndex];

    vec3 center = currentObject.center * currentObject.scale  + currentObject.pos;
    float radius = currentObject.radius * currentObject.scale;
    bool visible = true;
    for(uint i = 0; i < 6; ++i)
    {
        visible = visible && dot(shaderData.frustumData[i], vec4(center, 1)) > - radius;
    }

    if(visible && currentVisibility == 0)
    {
        // With each element that is added to the draw list, increment the count
        uint drawIndex = atomicAdd(bufferAddrs.indirectCount.drawCount, 1);

        IndirectOffsets currentRead = bufferAddrs.indirectBuffer.offsets[objectIndex];

        // The level of detail index that should be used is derived by the distance fromt he camera
        float lodDistance = log2(max(1, (distance(center, vec3(shaderData.viewPosition)) - radius)));
	    uint lodIndex = clamp(int(lodDistance), 0, int(currentRead.lodCount) - 1);

        MeshLod currentLod = currentRead.lod[lodIndex];

        // The object index is needed to know which element to access in the per object data buffer
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].objectId = objectIndex;

        // Indirect draw commands
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].indexCount = currentLod.indexCount;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].instanceCount = 1;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstIndex = currentLod.firstIndex;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].vertexOffset = currentRead.vertexOffset;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstInstance = 0;
    }

    // Tell the next frame if the object was visible last frame
    bufferAddrs.visibilityBuffer.visibilities[objectIndex] = visible ? 1 : 0;
}