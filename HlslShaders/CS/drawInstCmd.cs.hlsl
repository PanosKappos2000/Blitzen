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
    if(lodBuffer[lodId].instanceCount == 0)
    {
        return;
    }
    // Command count
    uint cmdId;
    InterlockedAdd(drawCountBuffer[0], 1, cmdId);

    // Render object id constant
    drawCmdBuffer[cmdId].objId = instanceCounterBuffer[lodId].instanceOffset;

    // Vertices
    drawCmdBuffer[cmdId].indexCount = lodBuffer[lodId].indexCount;
    drawCmdBuffer[cmdId].indexOffset = lodBuffer[lodId].indexOffset;
    drawCmdBuffer[cmdId].vertOffset = 0; // Already added to the index buffer

    // Instances
    drawCmdBuffer[cmdId].instCount = instanceCounterBuffer[lodId].instanceCount;
    drawCmdBuffer[cmdId].insOffset = 0;
}