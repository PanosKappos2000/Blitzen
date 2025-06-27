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
StructuredBuffer<ClusterVertices> ssbo_ClusterVertices : register(t6);

struct ClusterSphere
{
    float3 center;
    float radius;
};
StructuredBuffer<ClusterSphere> ssbo_ClusterSpheres : register(t7);

struct ClusterCone
{
    float3 cone;
    float coneCutoff;
};
StructuredBuffer<ClusterCone> ssbo_ClusterCones : register(t8);

cbuffer ObjCountConstant : register(b8)
{
    uint objCount;
};