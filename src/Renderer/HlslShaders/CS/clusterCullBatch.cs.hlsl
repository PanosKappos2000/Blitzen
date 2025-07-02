#define CLUSTER_CULL

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/clusterCull.hlsl"
#include "../../Resources/blitShaderShared.h"


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
                ssbo_DrawCmd[cmdId + BLIT_OPAQUE_STATIC_RENDER_OFFSET].indexCount += ssbo_ClusterVertices[groupData.clusterOffset + i].idxCount;
            }
            else
            {
                InterlockedAdd(rwb_DrawCmdCounter[0], 1, cmdId);
                
                ssbo_DrawCmd[cmdId + BLIT_OPAQUE_STATIC_RENDER_OFFSET].objId = groupData.objId;
                
                ssbo_DrawCmd[cmdId + BLIT_OPAQUE_STATIC_RENDER_OFFSET].indexCount = ssbo_ClusterVertices[groupData.clusterOffset + i].idxCount;
                ssbo_DrawCmd[cmdId + BLIT_OPAQUE_STATIC_RENDER_OFFSET].indexOffset = ssbo_ClusterVertices[groupData.clusterOffset + i].idxOffset;
                ssbo_DrawCmd[cmdId + BLIT_OPAQUE_STATIC_RENDER_OFFSET].vertOffset = 0;
                
                ssbo_DrawCmd[cmdId + BLIT_OPAQUE_STATIC_RENDER_OFFSET].instCount = 1;
                ssbo_DrawCmd[cmdId + BLIT_OPAQUE_STATIC_RENDER_OFFSET].insOffset = 0;
            }
            visible = 1;
        }
        else
        {
            visible = 0;
        }
    }

}