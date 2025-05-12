#define DRAW_INSTANCING
#include "../Headers/cullBuffers.hlsl"

cbuffer LodCount : register(b2)
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
    
    instanceCounterBuffer[lodId].instanceCount = 0;
}