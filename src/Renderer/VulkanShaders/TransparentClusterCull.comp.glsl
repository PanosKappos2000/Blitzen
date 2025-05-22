#version 450
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : enable

#define COMPUTE_PIPELINE
#define CLUSTER_CULLING
#include "../VulkanShaderHeaders/ShaderBuffers.glsl"
#include "../VulkanShaderHeaders/CullingShaderData.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    
    uint objectIndex = gl_GlobalInvocationID.x;
    if(pushConstant.drawCount <= objectIndex)
    {
        return;
    }
    ClusterDispatchData data = pushConstant.clusterDispatchBuffer.data[objectIndex];
    RenderObject obj = pushConstant.renderObjectBuffer.objects[data.objectId];
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
    // Get the selected LOD
    Lod currentLod = lodBuffer.levels[data.lodIndex];
    // The object index is needed to know which element to access in the per object data buffer
    indirectDrawBuffer.draws[drawID].objectId = data.objectId;
    // Setup the indirect draw commands based on the selected LODs and the vertex offset of the current surface
    indirectDrawBuffer.draws[drawID].indexCount = clusterBuffer.clusters[data.clusterId].triangleCount * 3;
    indirectDrawBuffer.draws[drawID].instanceCount = 1;
    indirectDrawBuffer.draws[drawID].firstIndex = clusterBuffer.clusters[data.clusterId].dataOffset;
    indirectDrawBuffer.draws[drawID].vertexOffset =  0;
    indirectDrawBuffer.draws[drawID].firstInstance = 0;
}