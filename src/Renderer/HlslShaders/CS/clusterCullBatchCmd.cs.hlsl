#define CLUSTER_CULL

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    rwssbo_ClusterDispatch[0].groupX = rwssbo_ClusterDispatch[0].groupX / 64 + 1;
    rwssbo_ClusterDispatch[0].groupY = 1;
    rwssbo_ClusterDispatch[0].groupZ = 1;
}