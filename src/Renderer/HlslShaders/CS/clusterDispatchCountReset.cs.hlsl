#define CLUSTER_CULL

#include "../Headers/cullBuffers.hlsl"

[numthreads(1, 1, 1)]
void csMain()
{
    rwb_ClusterDispatchCounter[0] = 0;
    rwssbo_ClusterDispatch[0].groupX = 0;
}