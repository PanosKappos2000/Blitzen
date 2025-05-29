#define CLUSTER_CULL

#include "../Headers/sharedBuffers.hlsl"
#include "../Headers/cullBuffers.hlsl"
#include "../Headers/hlslMath.hlsl"

cbuffer ObjCountConstant : register(b1)
{
    uint objCount;
};

[numthreads(64, 1, 1)]
void csMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 dispatchGroupID : SV_GroupID)
{
    uint objId = dispatchThreadID.x;
    // Early return if it's out of bounds
    if (objId >= objCount)
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
    
    if (visible)
    {
        uint lodId = LODSelection(center, radius, transform.scale, lodTarget, surface.lodOffset, surface.lodCount);

        uint clusterOffset = ssbo_LODs[lodId].clusterOffset;
        uint clusterCount = ssbo_LODs[lodId].clusterCount;
        
        uint dataId;
        InterlockedAdd(rwb_ClusterDispatchCounter[0], clusterCount, dataId);

        // Root constant data
        for (uint i = 0; i < clusterCount; i++)
        {
            rwssbo_ClusterGroupData[i + dataId].objId = objId;
            rwssbo_ClusterGroupData[i + dataId].clusterOffset = clusterOffset + i;
            rwssbo_ClusterGroupData[i + dataId].clusterCount = clusterCount;
            
            ++dataId;
        }
    }
}