#define CLUSTER_CULL

#include "../Headers/cullBuffers.hlsl"
#include "../Headers/clusterCull.hlsl"

[numthreads(1, 1, 1)]
void csMain()
{
    rwssbo_ClusterDispatch[0].groupX = 0;
}