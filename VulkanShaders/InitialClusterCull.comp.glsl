#version 450
#extension GL_GOOGLE_include_directive : require

#define COMPUTE_PIPELINE
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint dispatchIndex = gl_GlobalInvocationID.x;

    if (dispatchIndex >= cullPC.drawCount)
        return;

    ClusterDispatchCommand data = indirectClusterDispatch.commands[dispatchIndex];
    uint objectIndex = data.objectId;
    uint lodIndex = data.lodIndex;
    uint clusterLocalIndex = 0;//data.clusterLocalIndex;

    RenderObject obj = cullPC.renderObjectBufferAddress.objects[objectIndex];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];
    Surface surface = surfaceBuffer.surfaces[obj.surfaceId];

    MeshLod lod = surface.lod[lodIndex];
    uint clusterGlobalIndex = lod.meshletOffset + clusterLocalIndex;
    Cluster cluster = clusterBuffer.clusters[clusterGlobalIndex];

    uint drawIndex = dispatchIndex; // Direct mapping (1:1) for now

    indirectDrawBuffer.draws[drawIndex].objectId = objectIndex;
    indirectDrawBuffer.draws[drawIndex].indexCount = cluster.triangleCount * 3;
    indirectDrawBuffer.draws[drawIndex].instanceCount = 1;
    indirectDrawBuffer.draws[drawIndex].firstIndex = cluster.dataOffset + lod.firstIndex;
    indirectDrawBuffer.draws[drawIndex].vertexOffset = surface.vertexOffset;
    indirectDrawBuffer.draws[drawIndex].firstInstance = 0;
}