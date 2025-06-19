#define CLUSTER_CULL
#define HI_Z_MAP_OCCLUSION

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"
#include "../Headers/cpuShared.h"

cbuffer ObjCountConstant : register(b1)
{
    uint objCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x + BLIT_OPAQUE_STATIC_RENDER_OFFSET;
    
    // Early return if it's out of bounds
    if (objId >= objCount + BLIT_OPAQUE_STATIC_RENDER_OFFSET)
    {
        return;
    }
    
    Render obj = ssbo_Renders[objId];
    Surface surface = ssbo_Surfaces[obj.surfaceId];
    Transform transform = ssbo_Transforms[obj.transformId];

    // Promotes the bounding sphere's center to model and the view coordinates (frustum culling will be done on view space)
    float3 center = RotateQuat(surface.center, transform.orientation) * transform.scale + transform.position;
    center = mul(viewMatrix, float4(center, 1)).xyz;
    float radius = surface.radius * transform.scale;

    // Frustum culling
    bool visible = FrustumCheck(center, radius, frustumRight, frustumLeft, frustumTop, frustumBottom, zNear, zFar);
    
    // Occlusion culling
    if (visible)
    {
        float4 aabb = float4(0.f, 0.f, 0.f, 0.f);
        if (ProjectSphere(center, radius, zNear, proj0, proj5, aabb))
        {
            visible = OcclusionCheck(aabb, tex_HiZMap, pyramidWidth, pyramidHeight, center, radius, zNear);
        }
    }
    
    if (visible)
    {
        uint lodId = LODSelection(center, radius, transform.scale, lodTarget, surface.lodOffset, surface.lodCount);

        uint clusterOffset = ssbo_LODs[lodId].clusterOffset;
        uint clusterCount = ssbo_LODs[lodId].clusterCount;
        
        int currentClusterCount = clusterCount;
        uint currentLoop = 0;

        // Splits the clusters of the visible render object into groups of 64
        while (currentClusterCount > 0)
        {
            // Increments group count and gets current group(the final result will be used to dispatch the next shader)
            uint groupID;
            InterlockedAdd(rwssbo_ClusterDispatch[0].groupX, 1, groupID);
            
            // Render object ID, for access to transform data
            rwssbo_ClusterGroupData[groupID].objId = objId;
            
            // The cluster offset is incremented by 64 for each group created for this render object
            // Each individual cluster will be accessed using the thread ID
            rwssbo_ClusterGroupData[groupID].clusterOffset = clusterOffset + currentLoop * 64;
            
            // Cluster count is not the cluster count of this group but the maxID that this group is allowed to access
            rwssbo_ClusterGroupData[groupID].clusterCount = clusterCount + clusterOffset;
            
            // Total visibility for this group set to 0. If this remains 0 after cluster culling, this group will be skipped completely during batching
            rwssbo_ClusterGroupData[groupID].visibleAny = 0;
            
            // Loops again if there were more than 64 clusters remaining during this loops
            currentClusterCount -= 64;
            currentLoop++;
        }
    }
}