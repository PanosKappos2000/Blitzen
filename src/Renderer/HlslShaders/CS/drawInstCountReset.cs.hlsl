#define INSTANCED_CULL
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/instCull.hlsl"

[numthreads(8, 1, 1)]
void csMain(uint3 threadID : SV_GroupThreadID)
{
    rwb_DrawCmdCounter[threadID.x] = 0;
}