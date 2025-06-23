struct ClusterGroupData
{
    uint objectId;
    uint lodIndex;
    uint clusterId;

    uint padding0;
};

#ifdef PRE_CLUSTER
layout(set = 0, binding = 11, std430) writeonly buffer SSBO_CLUSTER_DISPATCH
{
	ClusterGroupData data[];
}rwssbo_ClusterGroup;
#else
layout(set = 0, binding = 11, std430) readonly buffer SSBO_CLUSTER_DISPATCH
{
	ClusterGroupData data[];
}rwssbo_ClusterGroup;
#endif

layout(set = 0, binding = 13, std430) writeonly buffer RWSSBO_CLUSTER_COUNT
{
	uint data;
}rwssbo_ClusterCount;

/* TODO: Maybe put a define in transparent culling and then add this */
//layout(set = 0, binding = 20, std430) writeonly buffer RWSSBO_CLUSTER_COUNT_TRANSPARENT
//{
//    uint data;
//}rwssbo_ClusterCountTransparent;

// Meshlet used in the mesh shader to draw a surface or mesh
struct Cluster
{
    // Bounding sphere for frustum culling
    vec3 center;
    float radius;

    // This is for backface culling
    int8_t coneAxisX;
    int8_t coneAxisY;
    int8_t coneAxisZ;
    int8_t coneCutoff;

    uint dataOffset; // Index into meshlet data
    uint8_t vertexCount;
    uint8_t triangleCount;
    uint8_t padding0;
    uint8_t padding1;
};

// The single buffer that holds all meshlet data in the scene
layout(set = 0, binding = 12, std430) readonly buffer SSBO_CLUSTER
{
    Cluster data[];
}ssbo_Cluster;