#define CLUSTER_CULL

#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

[numthreads(1, 1, 1)]
void csMain()
{
    rwssbo_ClusterDispatch[0].groupX = GetComputeShaderGroupSize(rwb_ClusterDispatchCounter[0], 64);
    rwssbo_ClusterDispatch[0].groupY = 1;
    rwssbo_ClusterDispatch[0].groupZ = 1;
}