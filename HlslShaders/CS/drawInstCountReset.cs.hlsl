#define DRAW_INSTANCING
#include "../Headers/cullBuffers.hlsl"

cbuffer LodCount : register(b1)
{
    uint lodCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint lodId = dispatchThreadID.x;
    if(lodId >= lodCount)
    {
        return;
    }
    lodBuffer[lodId].instanceCount = 0;
}