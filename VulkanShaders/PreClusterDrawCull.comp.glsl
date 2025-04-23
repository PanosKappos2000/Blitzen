#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_debug_printf : enable

#define COMPUTE_PIPELINE
#define PRE_CLUSTER
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout (push_constant) uniform PushConstants
{
    RenderObjectBuffer renderObjectBuffer;
    ClusterDispatchBuffer clusterDispatchBuffer;
    ClusterCountBuffer clusterCountBuffer;
    uint drawCount;
}pushConstant;

void main()
{
    uint objectIndex = gl_GlobalInvocationID.x;
    if (pushConstant.drawCount <= objectIndex)
    {
        return;
    }
    RenderObject obj = pushConstant.renderObjectBuffer.objects[objectIndex];
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
        uint lodOffset = surfaceBuffer.surfaces[obj.surfaceId].lodOffset;
        uint lodCount = surfaceBuffer.surfaces[obj.surfaceId].lodCount;
        uint lodIndex = LODSelection(center, radius, transform.scale, viewData.lodTarget, lodOffset, lodCount);
        lodIndex += lodOffset;
        Lod lod = lodBuffer.levels[lodIndex];

        uint dispatchIndex = atomicAdd(pushConstant.clusterCountBuffer.count, lod.clusterCount);
        for(uint i = 0; i < lod.clusterCount; ++i)
        {
            pushConstant.clusterDispatchBuffer.data[i + dispatchIndex].clusterId = lod.clusterOffset + i;
            pushConstant.clusterDispatchBuffer.data[i + dispatchIndex].lodIndex = lodIndex;
            pushConstant.clusterDispatchBuffer.data[i + dispatchIndex].objectId = objectIndex;
        }
    }
}