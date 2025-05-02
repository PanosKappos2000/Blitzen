#include "HlslShaders/sharedBuffers.hlsl"
#include "HlslShaders/cullBuffers.hlsl"

RWBuffer<uint> drawCountBuffer : register(u1);

cbuffer ObjCountConstant: register (b2)
{
    uint objCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint objId = dispatchThreadID.x;

    if(objId >= objCount)
    {
        return;
    }

    Render obj = renderBuffer[objId];
    Surface surface = surfaceBuffer[obj.surfaceId];

    uint cmdId = drawCountBuffer[0]; 
    InterlockedAdd(drawCountBuffer[0], 1);
    drawCmdBuffer[cmdId].drawId = objId;
    drawCmdBuffer[cmdId].indexCount = lodBuffer[0 + surface.lodOffset].indexCount;
    drawCmdBuffer[cmdId].instCount = 1;
    drawCmdBuffer[cmdId].indexOffset = lodBuffer[0 + surface.lodOffset].indexOffset;
    drawCmdBuffer[cmdId].vertOffset = 0;
    drawCmdBuffer[cmdId].insOffset = 0;
}