#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE

#include "ShaderBuffers.glsl.h"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (set = 0, binding = 2) uniform CullingData
{
    // Used for frustum culling
    vec4 frustumPlanes[6];

    float proj0;
    float proj5;
    float zNear;

    // Occulusion culling depth pyramid data
    float pyramidWidth;
    float pyramidHeight;

    // Debug values, to activate various acceleration algorithms
    uint occlusion;
    uint lod;

}cullingData;

void main()
{
    // The object index is for the current object's element in the render object and indirect offsets arrays
	uint objectIndex = gl_GlobalInvocationID.x;

    if(bufferAddrs.visibilityBuffer.visibilities[objectIndex] == 0)
        return;

    RenderObject currentObject = bufferAddrs.renderObjects.renderObjects[objectIndex];

    vec3 center = RotateQuat(currentObject.center, currentObject.orientation) * currentObject.scale + currentObject.pos;
    center = (shaderData.view * vec4(center, 1)).xyz;
	float radius = currentObject.radius * currentObject.scale;
	bool visible = true;
	for (int i = 0; i < 6; ++i)
		visible = visible && dot(cullingData.frustumPlanes[i], vec4(center, 1)) > -radius;
    
    if(visible)
    {
        // With each element that is added to the draw list, increment the count
        uint drawIndex = atomicAdd(bufferAddrs.indirectCount.drawCount, 1);

        IndirectOffsets currentRead = bufferAddrs.indirectBuffer.offsets[objectIndex];

        // The level of detail index that should be used is derived by the distance fromt he camera
        float lodDistance = log2(max(1, (distance(center, vec3(0)) - radius)));
	    uint lodIndex = clamp(int(lodDistance), 0, int(currentRead.lodCount) - 1);

        MeshLod currentLod = cullingData.lod == 1 ? currentRead.lod[lodIndex] : currentRead.lod[0];

        // The object index is needed to know which element to access in the per object data buffer
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].objectId = objectIndex;

        // Indirect draw commands
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].indexCount = currentLod.indexCount;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].instanceCount = 1;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstIndex = currentLod.firstIndex;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].vertexOffset = currentRead.vertexOffset;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstInstance = 0;
    }
    
}