#ifdef CLUSTER_CULL

struct ClusterDispatchCmd
{
    uint groupX;
    uint groupY;
    uint groupZ;

    uint padding0;
};
RWStructuredBuffer<ClusterDispatchCmd> rwssbo_ClusterDispatch : register(u5);

RWStructuredBuffer<uint> rwb_ClusterVisibility : register(u6);

struct ClusterGroupData
{
    uint objId;
    uint clusterOffset;
    uint clusterCount;
    uint visibleAny;
};
RWStructuredBuffer<ClusterGroupData> rwssbo_ClusterGroupData : register(u7);

struct ClusterVertices
{
    uint idxOffset;
    uint idxCount;
};
StructuredBuffer<ClusterVertices> ssbo_ClusterVertices : register(t8);

struct ClusterSphere
{
    float3 center;
    float radius;
};
StructuredBuffer<ClusterSphere> ssbo_ClusterSpheres : register(t9);

struct ClusterCone
{
    float3 cone;
    float coneCutoff;
};
StructuredBuffer<ClusterCone> ssbo_ClusterCones : register(t10);

#endif

struct DrawCmd
{
    // Index into render object buffer
    uint objId;

    // Draw command
    uint indexCount;
    uint instCount;
    uint indexOffset;
    int vertOffset;
    uint insOffset;

    uint padding0;
    uint padding1;
};
RWStructuredBuffer<DrawCmd> ssbo_DrawCmd : register(u0);

RWBuffer<uint> rwb_DrawCmdCounter : register(u1);

struct Lod 
{
    // Non cluster path, used to create draw commands
    uint indexCount;
    uint indexOffset;

    // Cluster path, used to create draw commands
    uint clusterOffset;
    uint clusterCount;

    // Used for more accurate LOD selection
    float error;

    // Pad to 32 bytes total 
    uint padding0;  
    uint padding1;
    uint padding3;
};
StructuredBuffer<Lod> ssbo_LODs : register (t7);

// The LOD index is calculated using a formula, where the distance to the bounding sphere's surface is taken
// and the minimum error that would result in acceptable screen-space deviation is computed based on camera parameters
uint LODSelection(float3 center, float radius, float scale, float lodTarget, uint lodOffset, uint lodCount)
{
    float distance = max(length(center) - radius, 0);
	float threshold = distance * lodTarget / scale;
    uint lodIndex = 0;
	for (uint i = 1; i < lodCount; ++i)
    {
		if (ssbo_LODs[lodOffset + i].error < threshold)
        {
			lodIndex = i;
        }
    }
    return lodOffset + lodIndex;
}

#ifdef DRAW_INSTANCING

struct InstanceCounter
{
    uint instanceOffset;
    uint instanceCount;
};
RWStructuredBuffer<InstanceCounter> rwssbo_InstCounter : register (u2);

#endif

#ifdef DRAW_CULL_OCCLUSION

RWStructuredBuffer<uint> rwssbo_DrawVisibilityBuffer : register (u5);

#endif

#ifdef HI_Z_MAP_OCCLUSION

Texture2D<float4> tex_HiZMap : register (t10);

#endif