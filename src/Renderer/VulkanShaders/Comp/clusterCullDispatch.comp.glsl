#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable

#define COMPUTE_PIPELINE
#define CLUSTER_CULLING
#define PRE_CLUSTER
#include "../Headers/sharedBuffers.glsl"
#include "../Headers/cullBuffers.glsl"
#include "../Headers/math.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x + pushConstant.drawOffset;
    if (pushConstant.drawCount <= objectIndex + pushConstant.drawOffset)
    {
        return;
    }
    
    RenderObject obj = ssbo_render.data[objectIndex];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];

    vec3 center;
    float radius;
    bool visible = CheckFrustum(center, radius, surfaceBuffer.surfaces[obj.surfaceId].center, surfaceBuffer.surfaces[obj.surfaceId].radius, transform.scale, transform.pos, transform.orientation,
        viewData.view, viewData.frustumRight, viewData.frustumLeft, viewData.frustumTop, viewData.frustumBottom, viewData.zNear, viewData.zFar);

    if (visible)
    {
        uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, surfaceBuffer.surfaces[obj.surfaceId].lodOffset, surfaceBuffer.surfaces[obj.surfaceId].lodCount);

        uint clusterCount = ssbo_LODs.data[lodIndex].clusterCount;
        uint dispatchIndex = atomicAdd(rwssbo_cluster_count.data[pushConstant.drawOffset], clusterCount);
        for(uint i = 0; i < clusterCount; ++i)
        {
            rwssbo_cluster_group.data[i + dispatchIndex + pushConstant.clusterGroupOffset].clusterId = ssbo_LODs.data[lodIndex].clusterOffset + i;
            rwssbo_cluster_group.data[i + dispatchIndex + pushConstant.clusterGroupOffset].lodIndex = lodIndex;
            rwssbo_cluster_group.data[i + dispatchIndex + pushConstant.clusterGroupOffset].objectId = objectIndex;
        }
    }
}