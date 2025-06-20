#define DRAW_INSTANCING
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/instCull.hlsl"

[numthreads(8, 1, 1)]
void csMain(uint3 threadID : SV_GroupThreadID)
{
    rwssbo_InstDrawCmd[threadID.x].instCount = 0;

}