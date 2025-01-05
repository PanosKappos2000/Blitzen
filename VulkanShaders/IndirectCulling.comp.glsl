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
    // The object index is for the current object's element in the render object
	uint objectIndex = gl_GlobalInvocationID.x;

    // The early culling shader does not test objects that were not visible last frame
    if(bufferAddrs.visibilityBuffer.visibilities[objectIndex] == 0)
        return;

    // Access the object's data
    RenderObject currentObject = bufferAddrs.objectBuffer.objects[objectIndex];
    MeshInstance currentInstance = bufferAddrs.meshInstanceBuffer.instances[currentObject.meshInstanceId];
    Surface currentSurface = bufferAddrs.surfaceBuffer.surfaces[currentObject.surfaceId];

    // Promote the sphere center to view coordinates
    vec3 center = RotateQuat(currentSurface.center, currentInstance.orientation) * currentInstance.scale + currentInstance.pos;
    center = (shaderData.view * vec4(center, 1)).xyz;
    // Scale the sphere radius
	float radius = currentSurface.radius * currentInstance.scale;
	bool visible = true;
    // Check that the bounding sphere is inside the view frustum(frustum culling)
	for (int i = 0; i < 6; ++i)
		visible = visible && dot(cullingData.frustumPlanes[i], vec4(center, 1)) > -radius;
    
    // Add the object to the indirect buffer, only if it passed frustum culling
    if(visible)
    {
        // With each element that is added to the draw list, increment the count
        uint drawIndex = atomicAdd(bufferAddrs.indirectCount.drawCount, 1);

        // The level of detail index that should be used is derived by the distance fromt he camera
        float lodDistance = log2(max(1, (distance(center, vec3(0)) - radius)));
	    uint lodIndex = clamp(int(lodDistance), 0, int(currentSurface.lodCount) - 1);

        MeshLod currentLod = cullingData.lod == 1 ? currentSurface.lod[lodIndex] : currentSurface.lod[0];

        // The object index is needed to know which element to access in the per object data buffer
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].objectId = objectIndex;

        // Indirect draw commands
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].indexCount = currentLod.indexCount;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].instanceCount = 1;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstIndex = currentLod.firstIndex;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].vertexOffset = currentSurface.vertexOffset;
        bufferAddrs.finalIndirectBuffer.indirectDraws[drawIndex].firstInstance = 0;
    }
    
}