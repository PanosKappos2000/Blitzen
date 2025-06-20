#define CLUSTER_CULL

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/clusterCull.hlsl"


[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    ClusterGroupData groupData = rwssbo_ClusterGroupData[dispatchThreadID.x];
    
    if(groupData.visibleAny != 1)
    {
        return;
    }
    
    uint visible = 0;
    uint cmdId;
    for (uint i = 0; i < 64; ++i)
    {
        if (groupData.clusterOffset + i >= groupData.clusterCount)
        {
            return;
        }
        
        if (rwb_ClusterVisibility[dispatchThreadID.x * 64 + i] == 1)
        {
            if(visible == 1)
            {
                ssbo_DrawCmd[cmdId].indexCount += ssbo_ClusterVertices[groupData.clusterOffset + i].idxCount;
            }
            else
            {
                InterlockedAdd(rwb_DrawCmdCounter[0], 1, cmdId);
                
                ssbo_DrawCmd[cmdId].objId = groupData.objId;
                
                ssbo_DrawCmd[cmdId].indexCount = ssbo_ClusterVertices[groupData.clusterOffset + i].idxCount;
                ssbo_DrawCmd[cmdId].indexOffset = ssbo_ClusterVertices[groupData.clusterOffset + i].idxOffset;
                ssbo_DrawCmd[cmdId].vertOffset = 0;
                
                ssbo_DrawCmd[cmdId].instCount = 1;
                ssbo_DrawCmd[cmdId].insOffset = 0;
            }
            visible = 1;
        }
        else
        {
            visible = 0;
        }
    }

}