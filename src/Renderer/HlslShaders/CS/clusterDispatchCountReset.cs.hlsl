#define CLUSTER_DISPATCH

#include "../Headers/cullBuffers.hlsl"

[numthreads(1, 1, 1)]
void csMain()
{
    rwb_ClusterDispatchCounter[0] = 0;
}