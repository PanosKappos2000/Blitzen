#version 450
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : enable

#define COMPUTE_PIPELINE
#define CLUSTER_CULLING
#include "../Headers/sharedBuffers.glsl"
#include "../Headers/cullBuffers.glsl"
#include "../Headers/math.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    
    uint objectIndex = gl_GlobalInvocationID.x + pushConstant.clusterGroupOffset;
    if(pushConstant.drawCount <= objectIndex)
    {
        return;
    }
    ClusterGroupData data = rwssbo_cluster_group.data[objectIndex];
    RenderObject obj = ssbo_render.data[data.objectId];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];

    // TEMP: Hardcoded camera position, replace later
    /*const vec3 cameraPosition = vec3(0.0, 0.0, -10.0);
    // Estimate cluster center (you can replace this if you already have it precomputed)
    // Decode the normalized cone axis from packed int8
    vec3 coneAxis = normalize(vec3(
        float(clusterBuffer.clusters[data.clusterId].coneAxisX), 
        float(clusterBuffer.clusters[data.clusterId].coneAxisY), 
        float(clusterBuffer.clusters[data.clusterId].coneAxisZ))
    );
    // Direction from cluster center to camera
    vec3 cameraVec = normalize(cameraPosition - clusterBuffer.clusters[data.clusterId].center);
    // Dot product with cone axis
    float facing = dot(coneAxis, cameraVec);
    float coneAngleCosine = float(clusterBuffer.clusters[data.clusterId].coneCutoff) / 127.0;
    // Skip cluster if it is backfacing
    if (facing < -coneAngleCosine)
    {
        return;
    }*/

    uint drawID = atomicAdd(indirectDrawCountBuffer.drawCount, 1);

    rwssbo_DrawCmd.data[drawID].objectId = data.objectId;

    // Vertices
    rwssbo_DrawCmd.data[drawID].indexCount = ssbo_cluster.data[data.clusterId].triangleCount * 3;
    rwssbo_DrawCmd.data[drawID].firstIndex = ssbo_cluster.data[data.clusterId].dataOffset;
    rwssbo_DrawCmd.data[drawID].vertexOffset =  0;

    // Instances
    rwssbo_DrawCmd.data[drawID].instanceCount = 1;
    rwssbo_DrawCmd.data[drawID].firstInstance = 0;
}