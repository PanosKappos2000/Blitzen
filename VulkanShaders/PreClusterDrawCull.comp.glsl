#version 450
#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE
#define PRE_CLUSTER
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x;
    if (cullPC.drawCount <= objectIndex)
    {
        return;
    }
    // This shader only processes objects that were visible last frame
    // Temporary disabled this is the only pre shader that will be called for now
    /*if(visibilityBuffer.visibilities[objectIndex] == 0)
    {
        return;
    }*/
    RenderObject obj = cullPC.renderObjectBufferAddress.objects[objectIndex];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];

    vec3 center;
    float radius;
    bool visible = IsObjectInsideViewFrustum(center, radius, 
        surfaceBuffer.surfaces[obj.surfaceId].center, surfaceBuffer.surfaces[obj.surfaceId].radius,
        transform.scale, transform.pos, transform.orientation,
        viewData.view, viewData.frustumRight, viewData.frustumLeft,
        viewData.frustumTop, viewData.frustumBottom, viewData.zNear, viewData.zFar);

    if (visible)
    {
        uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, 
            surfaceBuffer.surfaces[obj.surfaceId].lodOffset, surfaceBuffer.surfaces[obj.surfaceId].lodCount);
        Lod lod = lodBuffer.levels[lodIndex];

        uint dispatchIndex = atomicAdd(indirectClusterCount.count, lod.clusterCount);
        for(uint i = 0; i < lod.clusterCount; ++i)
        {
            indirectClusterDispatch.data[i + dispatchIndex].clusterId = lod.clusterOffset + i;
            indirectClusterDispatch.data[i + dispatchIndex].lodIndex = lodIndex;
            indirectClusterDispatch.data[i + dispatchIndex].objectId = objectIndex;
        }
    }
}