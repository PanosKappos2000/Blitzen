#define CLUSTER_CULL
#define HI_Z_MAP_OCCLUSION

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/clusterCull.hlsl"
#include "../Headers/hlslMath.hlsl"

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    // General dispatch accesses group of clusters
    ClusterGroupData groupData = rwssbo_ClusterGroupData[dispatchGroupID.x];

    // Individual clusters in group are accessed using the thread ID
    uint clusterId = groupData.clusterOffset + groupThreadID.x;
    
    // If this group has less than 64 clusters it is set from the cluster dispatch to quit early
    if (clusterId >= groupData.clusterCount)
    {
        return;
    }
    
    Transform transform = ssbo_Transforms[ssbo_Renders[groupData.objId].transformId];
    ClusterSphere boundingSphere = ssbo_ClusterSpheres[clusterId];
    
    // Bounding sphere transform
    float3 center = RotateQuat(boundingSphere.center, transform.orientation) * transform.scale + transform.position;
    center = mul(viewMatrix, float4(center, 1)).xyz;
    float radius = boundingSphere.radius * transform.scale;
    
    // Frustum culling
    bool visible = FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar);
    
    /*if (visible)
    {
        float4 aabb = float4(0.f, 0.f, 0.f, 0.f);
        if(ProjectSphere(center, radius, zNear, proj0, proj5, aabb))
        {
            visible = OcclusionCheck(aabb, tex_HiZMap, pyramidWidth, pyramidHeight, center, radius, zNear);
        }
    }*/
    
    if (visible)
    {
        rwssbo_ClusterGroupData[dispatchGroupID.x].visibleAny = 1;
        rwb_ClusterVisibility[dispatchThreadID.x] = 1;
    }
    else
    {
        rwb_ClusterVisibility[dispatchThreadID.x] = 0;
    }
}