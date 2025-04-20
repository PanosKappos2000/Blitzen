#version 450
#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x;
    if(cullPC.drawCount <= objectIndex)
    {
        return;
    }
    ClusterDispatchData data = indirectClusterDispatch.data[objectIndex];
    RenderObject obj = cullPC.renderObjectBufferAddress.objects[data.objectId];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];

    // Frustum culling
    /*vec3 center;
	float radius;
	bool visible = IsObjectInsideViewFrustum(center, radius, 
        surfaceBuffer.surfaces[obj.surfaceId].center, surfaceBuffer.surfaces[obj.surfaceId].radius, // bounding sphere
        transform.scale, transform.pos, transform.orientation, // object transform
        viewData.view, // view matrix
        viewData.frustumRight, viewData.frustumLeft, // frustum planes
        viewData.frustumTop, viewData.frustumBottom, // frustum planes part 2
        viewData.zNear, viewData.zFar // zFar and zNear
    );*/

    uint drawID = atomicAdd(indirectDrawCountBuffer.drawCount, 1);
    // Get the selected LOD
    MeshLod currentLod = surfaceBuffer.surfaces[obj.surfaceId].lod[data.lodIndex];
    // The object index is needed to know which element to access in the per object data buffer
    indirectDrawBuffer.draws[drawID].objectId = data.objectId;
    // Setup the indirect draw commands based on the selected LODs and the vertex offset of the current surface
    indirectDrawBuffer.draws[drawID].indexCount = currentLod.indexCount;
    indirectDrawBuffer.draws[drawID].instanceCount = 1;
    indirectDrawBuffer.draws[drawID].firstIndex = currentLod.firstIndex;
    indirectDrawBuffer.draws[drawID].vertexOffset = surfaceBuffer.surfaces[obj.surfaceId].vertexOffset;
    indirectDrawBuffer.draws[drawID].firstInstance = 0;
}