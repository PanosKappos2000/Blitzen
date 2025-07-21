#define COLLIDER_CULL

#include "../Headers/cullBuffers.hlsl"

[numthreads(1, 1, 1)]
void csMain()
{
    rwb_DrawCmdCounter[0] = 0;
}