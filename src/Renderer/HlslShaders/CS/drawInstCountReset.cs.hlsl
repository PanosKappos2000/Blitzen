#define INSTANCED_CULL
#define INSTANCING
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/instCull.hlsl"

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchID : SV_DispatchThreadID)
{
    if(dispatchID.x >= workCount)
    {
        return;
    }
    
    rwssbo_DrawCmd[dispatchID.x].instanceOffset = 0;
}