#define CLUSTER_DISPATCH
#define CLUSTER_CULL
#define HI_Z_MAP_OCCLUSION

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

cbuffer ClusterIndicesConstant : register(b2)
{
    uint objId;
    uint clusterOffset;
    uint clusterCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint clusterId = clusterOffset + dispatchThreadID.x;
    if (clusterId >= clusterCount)
    {
        return;
    }
    
    Render render = ssbo_Renders[objId];
    Transform transform = ssbo_Transforms[render.transformId];
    Cluster cluster = ssbo_Clusters[clusterId];
    
    bool visible = true;
    
    if(visible)
    {
        uint cmdId;
        InterlockedAdd(rwb_DrawCmdCounter[0], 1, cmdId);

        ssbo_DrawCmd[cmdId].objId = objId;
        
        // Vertices
        ssbo_DrawCmd[cmdId].indexCount = cluster.vertexCount;
        ssbo_DrawCmd[cmdId].indexOffset = cluster.clusterOffset;
        ssbo_DrawCmd[cmdId].vertOffset = 0; // Already added to the index buffer

        // Instances
        ssbo_DrawCmd[cmdId].instCount = 1;
        ssbo_DrawCmd[cmdId].insOffset = 0;
    }
}