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
    if(visibilityBuffer.visibilities[objectIndex] == 0)
    {
        return;
    }
    RenderObject obj = cullPC.renderObjectBufferAddress.objects[objectIndex];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];
    Surface surface = surfaceBuffer.surfaces[obj.surfaceId];

    vec3 center;
    float radius;
    bool visible = IsObjectInsideViewFrustum(center, radius, surface.center, surface.radius,
        transform.scale, transform.pos, transform.orientation,
        viewData.view, viewData.frustumRight, viewData.frustumLeft,
        viewData.frustumTop, viewData.frustumBottom, viewData.zNear, viewData.zFar);

    if (visible)
    {
        uint lodIndex = 0;
        float distance = max(length(center) - radius, 0);
        float threshold = distance * viewData.lodTarget / transform.scale;
        for (uint i = 1; i < surface.lodCount; ++i)
        {
            if (surface.lod[i].error < threshold)
                lodIndex = i;
        }

        uint dispatchIndex = atomicAdd(indirectClusterCount.count, 1);
        // Placeholder dispatch command
        indirectClusterDispatch.commands[dispatchIndex].groupCountX = surface.lod[lodIndex].meshletCount;
        indirectClusterDispatch.commands[dispatchIndex].groupCountY = 1;
        indirectClusterDispatch.commands[dispatchIndex].groupCountZ = 1;
        indirectClusterDispatch.commands[dispatchIndex].objectId = objectIndex;
        indirectClusterDispatch.commands[dispatchIndex].lodIndex = lodIndex;
    }
}