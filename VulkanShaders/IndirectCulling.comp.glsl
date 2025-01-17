#version 450

#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE

#include "../VulkanShaderHeaders/ShaderBuffers.glsl"


layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (set = 0, binding = 2) uniform CullingData
{
    // frustum planes
    float frustumRight;
    float frustumLeft;
    float frustumTop;
    float frustumBottom;

    float proj0;// The 1st element of the projection matrix
    float proj5;// The 12th element of the projection matrix

    // The draw distance and zNear, needed for both occlusion and frustum culling
    float zNear;
    float zFar;

    // Occulusion culling depth pyramid data
    float pyramidWidth;
    float pyramidHeight;

    // Debug values
    uint occlusionEnabled;
    uint lodEnabled;

    // Taking the draw count to guard against dispatching for empty draws
    uint drawCount;
}cullingData;

void main()
{
    // The object index is for the current object's element in the render object
	uint objectIndex = gl_GlobalInvocationID.x;

    // Abort if the object index is above draw count
    if(cullingData.drawCount < objectIndex)
        return;

    // The early culling shader does not test objects that were not visible last frame
    if(bufferAddrs.visibilityBuffer.visibilities[objectIndex] == 0)
        return;

    // Access the object's data
    RenderObject currentObject = bufferAddrs.objectBuffer.objects[objectIndex];
    MeshInstance currentInstance = bufferAddrs.transformBuffer.instances[currentObject.meshInstanceId];
    Surface currentSurface = bufferAddrs.surfaceBuffer.surfaces[currentObject.surfaceId];

    // Promote the sphere center to view coordinates
    vec3 center = RotateQuat(currentSurface.center, currentInstance.orientation) * currentInstance.scale + currentInstance.pos;
    center = (shaderData.view * vec4(center, 1)).xyz;

    // Scale the sphere radius
	float radius = currentSurface.radius * currentInstance.scale;

    // Check that the bounding sphere is inside the view frustum(frustum culling)
	bool visible = true;
    // the left/top/right/bottom plane culling utilizes frustum symmetry to cull against two planes at the same time(Arseny Kapoulkine)
    visible = visible && center.z * cullingData.frustumLeft - abs(center.x) * cullingData.frustumRight > -radius;
	visible = visible && center.z * cullingData.frustumBottom - abs(center.y) * cullingData.frustumTop > -radius;
	// the near/far plane culling uses camera space Z directly
	visible = visible && center.z + radius > cullingData.zNear && center.z - radius < cullingData.zFar;
	
    
    // Add the object to the indirect buffer, only if it passed frustum culling
    if(visible)
    {
        // With each element that is added to the draw list, increment the count
        uint drawIndex = atomicAdd(bufferAddrs.indirectCount.drawCount, 1);

        // The level of detail index that should be used is derived by the distance from he camera
        float lodDistance = log2(max(1, (distance(center, vec3(0)) - radius)));
	    uint lodIndex = clamp(int(lodDistance), 0, int(currentSurface.lodCount) - 1);

        MeshLod currentLod = cullingData.lodEnabled == 1 ? currentSurface.lod[lodIndex] : currentSurface.lod[0];

        // The object index is needed to know which element to access in the per object data buffer
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].objectId = objectIndex;

        // Indirect draw commands
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].indexCount = currentLod.indexCount;
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].instanceCount = 1;
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].firstIndex = currentLod.firstIndex;
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].vertexOffset = currentSurface.vertexOffset;
        bufferAddrs.indirectDrawBuffer.draws[drawIndex].firstInstance = 0;

        // Indirect task commands
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].taskId = currentLod.firstMeshlet;
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].groupCountX = (currentLod.meshletCount + 31) / 32;
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].groupCountY = 1;
        bufferAddrs.indirectTaskBuffer.tasks[drawIndex].groupCountZ = 1;

    }
    
}