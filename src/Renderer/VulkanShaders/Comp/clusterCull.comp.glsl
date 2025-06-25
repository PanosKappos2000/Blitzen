#version 450
#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_debug_printf : enable

#define COMPUTE_PIPELINE
#define CLUSTER_CULLING
#include "../Headers/sharedBuffers.glsl"
#include "../Headers/cullBuffers.glsl"
#include "../Headers/math.glsl"
#include "../Headers/clusterBuffers.glsl"
#include "../Headers/bufferOffsets.glsl"

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

void main()
{
    uint groupIndex = gl_GlobalInvocationID.x;

    if(rwssbo_ClusterCount.data <= groupIndex)
    {
        return;
    }

    ClusterGroupData data = rwssbo_ClusterGroup.data[groupIndex];
    RenderObject obj = ssbo_render.data[data.objectId];
    Transform transform = transformBuffer.instances[obj.meshInstanceId];

    uint drawID = atomicAdd(rwssbo_DrawCount.data, 1);

    rwssbo_DrawCmd.data[drawID].objectId = data.objectId;

    // Vertices
    rwssbo_DrawCmd.data[drawID].indexCount = ssbo_Cluster.data[data.clusterId].triangleCount * 3;
    rwssbo_DrawCmd.data[drawID].firstIndex = ssbo_Cluster.data[data.clusterId].dataOffset;
    rwssbo_DrawCmd.data[drawID].vertexOffset =  0;

    // Instances
    rwssbo_DrawCmd.data[drawID].instanceCount = 1;
    rwssbo_DrawCmd.data[drawID].firstInstance = 0;
}