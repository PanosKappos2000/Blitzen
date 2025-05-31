#define CLUSTER_CULL
#define HI_Z_MAP_OCCLUSION

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

cbuffer ClusterCount : register(b1)
{
    uint clusterCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    ClusterGroupData groupData = rwssbo_ClusterGroupData[dispatchGroupID.x];

    uint clusterId = groupData.clusterOffset + groupThreadID.x;
    uint objId = groupData.objId;
    
    // Probaly unneccessary
    if (clusterId >= clusterCount)
    {
        return;
    }
    
    if (clusterId >= groupData.clusterCount)
    {
        return;
    }
    
    Render render = ssbo_Renders[objId];
    Transform transform = ssbo_Transforms[render.transformId];
    Cluster cluster = ssbo_Clusters[clusterId];
    
    float3 center = RotateQuat(cluster.center, transform.orientation) * transform.scale + transform.position;
    center = mul(viewMatrix, float4(center, 1)).xyz;
    float radius = cluster.radius * transform.scale;
    
    bool visible = FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar);
    
    /*if (visible)
    {
        float4 aabb = float4(0.f, 0.f, 0.f, 0.f);
        if(ProjectSphere(center, radius, zNear, proj0, proj5, aabb))
        {
            visible = OcclusionCheck(aabb, tex_HiZMap, pyramidWidth, pyramidHeight, center, radius, zNear);
        }
    }*/
    
    if(visible)
    {
        rwssbo_ClusterGroupData[dispatchGroupID.x].visibleAny = 1;
        rwb_ClusterVisibility[dispatchThreadID.x] = 1;
    }
    else
    {
        rwb_ClusterVisibility[dispatchThreadID.x] = 0;
    }
}