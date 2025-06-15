#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

#define LOD_ENABLED
#define OCCLUSION_ENABLED

struct Lod
{
    uint indexCount;
    uint firstIndex;

    uint clusterOffset;
    uint clusterCount;

    float error;

    // Pad to 32 bytes total
    uint padding0;
    uint padding1;
    uint padding2;
};

layout(set = 0, binding = 4, std430) readonly buffer SSBO_LOD
{
    Lod data[];
}ssbo_LODs;

uint LODSelection(vec3 center, float radius, float scale, float lodTarget, uint lodOffset, uint lodCount)
{
    float distance = max(length(center) - radius, 0);
	float threshold = distance * lodTarget / scale;
    uint lodIndex = 0;
	for (uint i = 1; i < lodCount; ++i)
    {
		if (ssbo_LODs.data[lodOffset + i].error < threshold)
        {
			lodIndex = i;
        }
    }
    return lodIndex + lodOffset;
}

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
}rwssbo_cluster_group;
#else
layout(set = 0, binding = 11, std430) readonly buffer SSBO_CLUSTER_DISPATCH
{
	ClusterGroupData data[];
}rwssbo_cluster_group;
#endif

layout(set = 0, binding = 13, std430) writeonly buffer RWSSBO_CLUSTER_COUNT
{
	uint data[];
}rwssbo_cluster_count;

// The indirect count buffer holds a single integer that is the draw count for VkCmdDrawIndexedIndirectCount. 
// Will be incremented when necessary by a compute shader
layout(set = 0, binding = 9, std430) writeonly buffer IndirectDrawCount
{
    uint drawCount;
}indirectDrawCountBuffer;

#ifdef DOUBLE_PASS

layout(set = 0, binding = 10, std430) buffer RWSSBO_DRAW_VIS
{
    uint data[];
}rwssbo_DrawVis;

#endif

#ifdef CLUSTER_CULLING

layout (push_constant) uniform PushConstants
{
    uint clusterGroupOffset;
    uint clusterCountOffset;
    uint drawOffset;
    uint drawCount;
}pushConstant;

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
}ssbo_cluster;

#else

layout (push_constant) uniform CullingConstants
{
    uint drawOffset;
    uint drawCount;
	uint padding0;
    uint padding1;
}pushConstant;

#endif