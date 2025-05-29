#define CLUSTER_CULL
#define HI_Z_MAP_OCCLUSION

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

cbuffer ClusterCount : register(b1)
{
    uint clusterCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    ClusterGroupData groupData = rwssbo_ClusterGroupData[dispatchGroupID.x];

    uint clusterId = groupData.clusterOffset + groupThreadID.x;
    uint objId = groupData.objId;
    
    if (clusterId >= clusterCount)
    {
        return;
    }
    
    if (clusterId >= groupData.clusterCount)
    {
        return;
    }
    
    Render render = ssbo_Renders[objId];
    Transform transform = ssbo_Transforms[render.transformId];
    uint vertexCount = ssbo_Clusters[clusterId].vertexCount;
    uint indexOffset = ssbo_Clusters[clusterId].clusterOffset;
    
    bool visible = true;
    
    if(visible)
    {
        uint cmdId;
        InterlockedAdd(rwb_DrawCmdCounter[0], 1, cmdId);

        ssbo_DrawCmd[cmdId].objId = objId;
        
        // Vertices
        ssbo_DrawCmd[cmdId].indexCount = vertexCount;
        ssbo_DrawCmd[cmdId].indexOffset = indexOffset;
        ssbo_DrawCmd[cmdId].vertOffset = 0; // Already added to the index buffer

        // Instances
        ssbo_DrawCmd[cmdId].instCount = 1;
        ssbo_DrawCmd[cmdId].insOffset = 0;
    }
}