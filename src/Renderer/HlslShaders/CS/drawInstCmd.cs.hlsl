#define DRAW_INSTANCING
#include "../Headers/cullBuffers.hlsl"

cbuffer LodCount : register(b2)
{
    uint lodCount;
}

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint lodId = dispatchThreadID.x;
    if(lodId >= lodCount)
    {
        return;
    }

    // Account for culling
    if(rwssbo_InstCounter[lodId].instanceCount == 0)
    {
        return;
    }

    // Command count
    uint cmdId;
    InterlockedAdd(rwb_DrawCmdCounter[0], 1, cmdId);

    // Render object id constant
    ssbo_DrawCmd[cmdId].objId = rwssbo_InstCounter[lodId].instanceOffset;

    // Vertices
    ssbo_DrawCmd[cmdId].indexCount = ssbo_LODs[lodId].indexCount;
    ssbo_DrawCmd[cmdId].indexOffset = ssbo_LODs[lodId].indexOffset;
    ssbo_DrawCmd[cmdId].vertOffset = 0; // Already added to the index buffer

    // Instances
    ssbo_DrawCmd[cmdId].instCount = rwssbo_InstCounter[lodId].instanceCount;
    ssbo_DrawCmd[cmdId].insOffset = 0;
}