#define CLUSTER_CULL

#include "../Headers/cullBuffers.hlsl"

[numthreads(1, 1, 1)]
void csMain()
{
    rwssbo_ClusterDispatch[0].groupX = 0;
    rwssbo_ClusterDispatch[0].groupY = 0;
    rwssbo_ClusterDispatch[0].groupZ = 0;
}