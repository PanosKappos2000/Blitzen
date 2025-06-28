#define OPAQUE_DYNAMIC_CULL

#include "../Headers/cullBuffers.hlsl"
#include "../Headers/dynamicCull.hlsl"

[numthreads(1, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    rwb_DrawCmdCounter[0] = 0;
}